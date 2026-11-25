"""Offline GitHub outage messages: DNS, TLS, HTTP, caching and recovery.

Exercises the same functions used by Advanced -> Check repository and the
per-device firmware catalog. No GitHub call, DNS query or OPNsense change.
"""
import json
import socket
import ssl
import sys
import time
import unittest
from pathlib import Path
from urllib.error import HTTPError, URLError
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"rflink_builder"))

from github_errors import github_failure, github_api_error, GitHubRetryPaused
from release_history import ReleaseHistory
import server


def release(n=40):
    return {"id":n+100, "tag_name":f"firmware/v0.7.0-exp.{n}",
            "name":f"Exp {n}", "published_at":"2026-11-23T10:20:00Z",
            "prerelease":True, "draft":False, "assets":[], "body":""}


def stable():
    return {"id":99, "tag_name":"firmware/v0.6.6", "name":"Stable",
            "published_at":"2026-10-01T11:00:00Z",
            "prerelease":False, "draft":False, "assets":[], "body":""}


class GitHubFailureMessageTests(unittest.TestCase):
    def test_dns_error_is_actionable_and_does_not_blame_opnsense_as_fact(self):
        error=URLError(socket.gaierror(-3,"Temporary failure in name resolution"))
        status, message=github_api_error(error)
        self.assertEqual(status,503)
        self.assertEqual(message["failure_kind"],"dns")
        self.assertIn("api.github.com",message["error"])
        self.assertIn("DNS",message["error"])
        self.assertNotIn("Temporary failure in name resolution",message["error"])

    def test_timeout_https_tls_and_socket_errors_are_distinct(self):
        cases=[
            (URLError(TimeoutError("timed out")), "timeout", "443"),
            (URLError(ssl.SSLError("secret proxy interception")), "tls", "TLS"),
            (URLError(ConnectionRefusedError(111,"refused")), "network", "443"),
            (URLError("mysterious internal proxy token=SECRET"), "connection", "GitHub")
        ]
        for error, kind, marker in cases:
            with self.subTest(kind=kind):
                info=github_failure(error)
                self.assertEqual(info["failure_kind"],kind)
                self.assertIn(marker,info["failure_message"])
                self.assertNotIn("SECRET",info["failure_message"])
                self.assertNotIn("secret proxy interception",info["failure_message"])

    def test_api_limit_not_confused_with_forbidden_repository(self):
        url="https://api.github.com/repos/owner/repo/releases"
        errors=[
            (HTTPError(url,429,"API quota",{"Retry-After":"240"},None),"rate_limit",429),
            (HTTPError(url,403,"private",{},None),"forbidden",502),
            (HTTPError(url,403,"quota",{"X-RateLimit-Remaining":"0",
                 "X-RateLimit-Reset":str(int(time.time())+150)},None),"rate_limit",429),
            (HTTPError(url,404,"not found",{},None),"not_found",502),
            (HTTPError(url,502,"bad gateway",{},None),"github_unavailable",503),
        ]
        for error,kind,expected_status in errors:
            with self.subTest(code=error.code,kind=kind):
                status,detail=github_api_error(error)
                self.assertEqual((status,detail["failure_kind"]),(expected_status,kind))
                self.assertNotIn(url,detail["error"])
        status,detail=github_api_error(errors[0][0])
        self.assertEqual(detail["retry_after_seconds"],240)
        self.assertIn("4 minute",detail["error"])

    def test_invalid_json_is_not_mistaken_for_network_or_empty_release(self):
        error=json.JSONDecodeError("value","not json",0)
        code,detail=github_api_error(error)
        self.assertEqual(code,502)
        self.assertEqual(detail["failure_kind"],"invalid_response")
        self.assertIn("proxy",detail["error"])


class ReleaseFailureRecoveryTests(unittest.TestCase):
    def test_stale_cached_versions_have_specific_error_and_recover(self):
        mode=["ok"]
        calls=[]
        def fetch(url,limit):
            calls.append(url)
            if mode[0]=="dns":
                raise URLError(socket.gaierror(-3,"name resolution"))
            return json.dumps([stable(),release()]).encode()
        history=ReleaseHistory(fetch=fetch,cache_seconds=900,manual_cooldown=120)
        first=history.page("owner/repo")
        self.assertFalse(first["stale"])
        self.assertEqual(len(first["releases"]),2)
        mode[0]="dns"
        history.last_attempt[("owner/repo",1)]=time.monotonic()-121
        stale=history.page("owner/repo",refresh=True)
        self.assertTrue(stale["stale"])
        self.assertTrue(stale["cached"])
        self.assertEqual(stale["failure_kind"],"dns")
        self.assertIn("DNS",stale["failure_message"])
        self.assertEqual(stale["releases"],first["releases"])
        paused=history.page("owner/repo",refresh=True)
        self.assertEqual(paused["failure_kind"],"dns")
        self.assertTrue(paused["stale"])
        self.assertEqual(len(calls),2)
        recent=history.recent("owner/repo")
        self.assertTrue(recent["stale"])
        self.assertEqual(recent["latest"]["tag"],"firmware/v0.6.6")
        self.assertEqual(recent["failure_kind"],"dns")
        # A later successful request must not remain marked stale.
        mode[0]="ok"
        history.blocked_until["owner/repo"]=time.monotonic()-1
        restored=history.page("owner/repo")
        self.assertFalse(restored["stale"])
        self.assertNotIn("failure_kind",restored)
        self.assertEqual(len(calls),3)

    def test_missing_cache_retries_show_actual_reason_and_remaining_delay(self):
        count=[0]
        def dns_failure(url,limit):
            count[0]+=1
            raise URLError(socket.gaierror(-3,"Temporary failure in name resolution"))
        history=ReleaseHistory(fetch=dns_failure)
        with self.assertRaises(URLError): history.page("owner/repo")
        with self.assertRaises(GitHubRetryPaused) as caught:
            history.page("owner/repo")
        status,body=github_api_error(caught.exception)
        self.assertEqual(status,503)
        self.assertEqual(body["failure_kind"],"dns")
        self.assertGreaterEqual(body["retry_after_seconds"],119)
        self.assertIn("Next check",body["error"])
        self.assertEqual(count[0],1) # no spamming GitHub on repeated clicks

    def test_check_repository_does_not_claim_no_stable_during_outage(self):
        mode=["ready"]
        def fetch(url,limit):
            if mode[0]=="offline":
                raise URLError(socket.gaierror(-3,"DNS offline"))
            return json.dumps([stable()]).encode()
        history=ReleaseHistory(fetch=fetch)
        history.page("owner/repo")
        mode[0]="offline"
        history.last_attempt[("owner/repo",1)]=time.monotonic()-121
        history.page("owner/repo",refresh=True)
        with patch.object(server,"github_configuration",return_value={"repo":"owner/repo"}), \
             patch.object(server,"github_head",return_value="a"*40), \
             patch.object(server,"release_history",history):
            result=server.github_status()
        self.assertTrue(result["release"]["available"])
        self.assertTrue(result["stale"])
        self.assertEqual(result["failure_kind"],"dns")
        self.assertEqual(result["release"]["version"],"firmware/v0.6.6")

    def test_ordinary_403_is_not_called_rate_limit(self):
        url="https://api.github.com/repos/owner/repo/releases"
        def denied(*_):
            raise HTTPError(url,403,"blocked",{},None)
        history=ReleaseHistory(fetch=denied)
        with self.assertRaises(HTTPError):
            history.page("owner/repo")
        self.assertEqual(history.last_failure["owner/repo"]["failure_kind"],"forbidden")
        self.assertLess(history.blocked_until["owner/repo"]-time.monotonic(),121)


class BuilderDisplayContracts(unittest.TestCase):
    def test_advanced_and_device_firmware_show_stale_reason(self):
        app=(ROOT/"rflink_builder/static/app.js").read_text()
        chooser=(ROOT/"rflink_builder/static/firmware_catalog.js").read_text()
        html=(ROOT/"rflink_builder/static/index.html").read_text()
        server_source=(ROOT/"rflink_builder/server.py").read_text()
        self.assertIn("if(r.stale)",app)
        self.assertIn("Repository check failed:",app)
        self.assertIn("Repository reachable · no published Stable",app)
        self.assertIn("r.failure_message",app)
        self.assertIn("response.failure_message",chooser)
        self.assertIn("last verified",chooser)
        self.assertIn('aria-live="polite"',html.split('id="repo-status"')[1][:80])
        self.assertIn("github_api_error(exc)",server_source)
        self.assertIn("releases.get('stale',False)",server_source)

if __name__=="__main__":unittest.main()
