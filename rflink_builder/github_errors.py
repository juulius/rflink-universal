"""Safe, actionable GitHub connectivity feedback for RFLink Builder.

Describe a failed *outbound Builder-to-GitHub* request. Never reflect arbitrary
HTTP response bodies, URL query strings, proxy credentials or TLS internals in
the browser. Errors are diagnostic hints, not proof of an OPNsense rule.
"""
from __future__ import annotations

import errno
import json
import socket
import ssl
from urllib.error import HTTPError, URLError


class GitHubRetryPaused(Exception):
    """Release-history backoff when no previously cached page is available."""

    def __init__(self, previous, seconds):
        super().__init__("GitHub release checks are temporarily paused")
        self.previous = previous or {}
        self.seconds = max(0, int(seconds))


def _response_headers(exc):
    return exc.headers or {}


def _rate_limited(exc):
    if exc.code == 429:
        return True
    if exc.code != 403:
        return False
    headers = _response_headers(exc)
    return headers.get("X-RateLimit-Remaining") == "0" or bool(headers.get("Retry-After"))


def github_failure(exc):
    """Return safe API fields: failure_kind, failure_message and HTTP status."""
    if isinstance(exc, GitHubRetryPaused):
        previous = exc.previous
        return {
            "failure_kind": previous.get("failure_kind") or "retry_paused",
            "failure_message": previous.get("failure_message") or
                "A recent GitHub request failed. Automatic retries are temporarily paused.",
            "retry_after_seconds": exc.seconds,
            "http_status": 429 if previous.get("failure_kind") == "rate_limit" else 503,
        }
    if isinstance(exc, HTTPError):
        code = exc.code
        if _rate_limited(exc):
            kind, status, message = "rate_limit", 429, (
                "GitHub's API request limit was reached. The Builder will retry after "
                "the cooldown; do not repeatedly refresh or change firewall rules."
            )
        elif code == 401:
            kind, status, message = "authentication", 502, (
                "GitHub rejected access (HTTP 401). Check repository visibility "
                "and API access."
            )
        elif code == 403:
            kind, status, message = "forbidden", 502, (
                "GitHub refused this request (HTTP 403). Check repository access "
                "or GitHub API restrictions; this does not establish a firewall block."
            )
        elif code == 404:
            kind, status, message = "not_found", 502, (
                "GitHub returned HTTP 404. Check the repository owner/name, "
                "visibility and whether its main branch or requested release exists."
            )
        elif 500 <= code <= 599:
            kind, status, message = "github_unavailable", 503, (
                "GitHub returned a server error (HTTP 5xx). Try again later; "
                "your OPNsense firewall is not necessarily involved."
            )
        else:
            kind, status, message = "http_error", 502, (
                "GitHub rejected the request (HTTP %d). Check the repository "
                "settings and retry." % code
            )
        return {"failure_kind": kind, "failure_message": message,
                "retry_after_seconds": 0, "http_status": status}
    if isinstance(exc, json.JSONDecodeError):
        return {"failure_kind": "invalid_response", "http_status": 502,
                "failure_message": "GitHub returned invalid JSON. Check whether a "
                "proxy or network filter is replacing the response, then retry.",
                "retry_after_seconds": 0}
    reason = exc.reason if isinstance(exc, URLError) else exc
    if isinstance(reason, ssl.SSLError):
        kind, message = "tls", (
            "The HTTPS certificate or TLS connection to GitHub could not be "
            "validated. Check the Builder's system clock, DNS and HTTPS inspection."
        )
    elif isinstance(reason, socket.gaierror) or (
        isinstance(reason, str) and any(marker in reason.lower() for marker in (
            "name or service not known", "temporary failure in name resolution",
            "nodename nor servname provided", "getaddrinfo failed"
        ))
    ):
        kind, message = "dns", (
            "The Builder could not resolve api.github.com. Check its DNS server "
            "and DNS permissions in Home Assistant / OPNsense."
        )
    elif isinstance(reason, (TimeoutError, socket.timeout)):
        kind, message = "timeout", (
            "GitHub did not respond within the connection timeout. Check "
            "outbound HTTPS (TCP 443), DNS, routing and any firewall blocks."
        )
    elif isinstance(reason, (ConnectionError, OSError)) and getattr(reason, "errno", None) in (
        errno.ENETUNREACH, errno.EHOSTUNREACH, errno.ECONNREFUSED,
        errno.ECONNRESET, errno.ENETDOWN, errno.EPIPE
    ):
        kind, message = "network", (
            "The Builder could not establish or maintain an outbound HTTPS "
            "connection to GitHub. Check routing, OPNsense firewall/NAT and TCP 443."
        )
    else:
        kind, message = "connection", (
            "The Builder could not contact GitHub. Check DNS and outbound HTTPS "
            "(TCP 443), then retry. This does not prove an OPNsense firewall block."
        )
    return {"failure_kind": kind, "failure_message": message,
            "retry_after_seconds": 0, "http_status": 503}


def github_api_error(exc):
    """Return (HTTP status, sanitized response) for a failed GitHub API call."""
    info = github_failure(exc)
    status = info.pop("http_status")
    return status, {"error": info["failure_message"], **info}
