"""GitHub Releases history and immutable native ESP-IDF source snapshots.

No shell commands, moving branches, GitHub release binaries, or secrets are
accepted as sources.  The release's tag is resolved to a full commit SHA before
fetching an ESP-IDF source archive, then the existing ZIP validator is reused.
"""
from __future__ import annotations

import json
import re
import threading
import time
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen
from urllib.parse import quote

from repo_sync import valid_repo, read_http, github_firmware_zip

TAG = re.compile(r'^[A-Za-z0-9][A-Za-z0-9._+\-/]{0,119}$')
SHA = re.compile(r'^[a-f0-9]{40}$')
MAX_PAGES = 20
PAGE_SIZE = 30


def clean_tag(tag):
    if not isinstance(tag, str) or not TAG.fullmatch(tag) or '..' in tag.split('/'):
        raise ValueError('Ongeldige GitHub-release-tag')
    return tag


def version_tuple(tag):
    """Comparison key for stable versions; pre-releases sort before stable."""
    found = re.fullmatch(r'(?:(?:firmware|fw)[/\-])?[vV]?(\d+)\.(\d+)(?:\.(\d+))?(?:-([0-9A-Za-z.\-]+))?', tag or '')
    if not found:
        return None
    x, y, z, pre = found.groups()
    return int(x), int(y), int(z or 0), 0 if pre else 1


def release_identity(tag, github_prerelease=False):
    """Human-readable channel, version and immutable per-channel build sequence.

    Each experimental / RC build has a *different tag*. The displayed firmware
    family stays the same, while GitHub tags and source commits never move.
    """
    match = re.fullmatch(
        r'(?:(?:firmware|fw)[/\-])?[vV]?(\d+)\.(\d+)(?:\.(\d+))?'
        r'(?:-([0-9A-Za-z.\-]+))?', tag or '')
    if not match:
        return None
    major, minor, patch, suffix = match.groups()
    version = f'v{int(major)}.{int(minor)}.{int(patch or 0)}'
    channel, build = 'stable', 0
    if suffix:
        experimental = re.fullmatch(r'(?:exp|experimental)[.\-]?(\d+)', suffix, re.I)
        candidate = re.fullmatch(r'(?:rc|pre|prerelease)[.\-]?(\d+)', suffix, re.I)
        if experimental:
            channel, build = 'experimental', int(experimental.group(1))
        elif candidate:
            channel, build = 'pre-release', int(candidate.group(1))
        else:
            channel = 'pre-release'
    elif github_prerelease:
        # A GitHub pre-release with a plain version must never appear as stable.
        channel = 'pre-release'
    label = {'stable': 'Stable', 'pre-release': 'Pre-release',
             'experimental': 'Experimental'}[channel]
    display = version + ' · ' + label
    if build:
        display += f' · build {build}' if channel == 'experimental' else f' · RC {build}'
    return {'version': version, 'channel': channel, 'build_number': build,
            'display': display}


def is_older(tag, reference):
    a = version_tuple(tag)
    b = version_tuple(reference)
    return a is not None and b is not None and a < b


def normalise_release(item):
    if not isinstance(item, dict) or item.get('draft') or not item.get('published_at'):
        return None
    tag = item.get('tag_name')
    try:
        clean_tag(tag)
        if version_tuple(tag) is None:  # ignore Builder-only GitHub Releases
            return None
    except ValueError:
        return None
    ident = item.get('id')
    if not isinstance(ident, int) or isinstance(ident, bool) or ident <= 0:
        return None
    identity = release_identity(tag, bool(item.get('prerelease')))
    if not identity:
        return None
    # Preview tags must be marked pre-release on GitHub. Never silently show
    # an experimental source snapshot as a verified stable release.
    if identity['channel'] != 'stable' and not item.get('prerelease'):
        return None
    assets = item.get('assets') or []
    return {
        'id': ident, 'tag': tag, **identity,
        'name': str(item.get('name') or tag)[:120],
        'published_at': str(item['published_at'])[:32],
        'prerelease': bool(item.get('prerelease')),
        'notes': str(item.get('body') or '')[:7000],
        'url': f"https://github.com/{item.get('_verified_repo', 'OWNER/REPO')}/releases/tag/{quote(tag, safe='')}",
        'assets': [str(x.get('name') or '')[:128] for x in assets if isinstance(x, dict)][:30],
    }


class ReleaseAutoChecker:
    """Check published releases hourly without creating another worker thread.

    Called by the Builder's existing device monitor. The device Firmware tab
    continues to refresh its own release list every 15 minutes while visible.
    The ReleaseHistory cache/ETag/rate-limit policy remains authoritative.
    """

    INTERVAL_SECONDS = 60 * 60

    def __init__(self, history, repository, clock=time.monotonic):
        self.history = history
        self.repository = repository
        self.clock = clock
        self.next_check_at = 0.0

    def check_if_due(self):
        now = self.clock()
        if now < self.next_check_at:
            return False
        # Record the next attempt before the network request, including errors,
        # to prevent rapid retries when GitHub is offline/rate-limited.
        self.next_check_at = now + self.INTERVAL_SECONDS
        repo = self.repository()
        if not repo:
            return False
        self.history.page(repo, 1)
        return True


class ReleaseHistory:
    def __init__(self, fetch=read_http, source_fetch=github_firmware_zip,
                 cache_seconds=900, manual_cooldown=120):
        self.fetch = fetch
        self.source_fetch = source_fetch
        self.cache_seconds = cache_seconds
        self.manual_cooldown = manual_cooldown
        self.cache = {}
        self.etags = {}
        self.last_attempt = {}
        self.last_checked = {}
        self.blocked_until = {}
        self.failures = {}
        # Serialize remote release list reads across browser tabs and requests.
        self.lock = threading.RLock()

    def _api(self, repo, suffix, limit=1_250_000):
        repo = valid_repo(repo)
        return json.loads(self.fetch(f'https://api.github.com/repos/{repo}/{suffix}', limit))

    def _release_page_http(self, repo, number, etag):
        """Conditional GET reduces transfer size. Unauthenticated 304 may still
        count towards GitHub's 60/hour/IP primary limit, hence the 15m cache.
        Test-injected fetch functions keep their original (url, limit) contract.
        """
        url = f'https://api.github.com/repos/{repo}/releases?per_page={PAGE_SIZE}&page={number}'
        if self.fetch is not read_http:
            return json.loads(self.fetch(url, 1_250_000)), None, {}
        headers = {'Accept': 'application/vnd.github+json',
                   'User-Agent': 'RFLink-Universal-Builder/0.6.25'}
        if etag:
            headers['If-None-Match'] = etag
        req = Request(url, headers=headers)
        try:
            with urlopen(req, timeout=18) as response:
                body = response.read(1_250_001)
                if len(body) > 1_250_000:
                    raise ValueError('GitHub release response exceeds size limit')
                return json.loads(body), response.headers.get('ETag'), response.headers
        except HTTPError as exc:
            if exc.code == 304:
                return None, etag, exc.headers
            raise

    @staticmethod
    def _rate_limit_delay(exc, failures):
        """Honor GitHub's Retry-After and exhausted primary-limit reset."""
        headers = exc.headers
        retry = headers.get('Retry-After') if headers else None
        if retry:
            try:
                return max(60, min(86400, int(retry)))
            except ValueError:
                pass
        remaining = headers.get('X-RateLimit-Remaining') if headers else None
        reset = headers.get('X-RateLimit-Reset') if headers else None
        if remaining == '0' and reset:
            try:
                return max(60, min(86400, int(reset) - int(time.time()) + 5))
            except ValueError:
                pass
        return min(3600, 60 * 2 ** min(failures, 6))

    @staticmethod
    def _sort_key(item):
        ver = version_tuple(item['tag'])
        return (ver[:3], item['build_number'], item['published_at'], item['id'])

    def page(self, repo, number=1, refresh=False):
        repo = valid_repo(repo)
        if not isinstance(number, int) or not 1 <= number <= MAX_PAGES:
            raise ValueError('Ongeldige pagina (1–20)')
        key = (repo, number)
        with self.lock:
            now = time.monotonic()
            cached = self.cache.get(key)
            blocked = max(0, int(self.blocked_until.get(repo, 0) - now + .999))
            if blocked:
                if cached:
                    return dict(cached[1], cached=True, stale=True,
                                retry_after_seconds=blocked,
                                checked_at=self.last_checked.get(key, 0))
                raise ValueError(f'GitHub release checks temporarily paused; retry in {blocked}s')
            since = now - self.last_attempt.get(key, -1e12)
            cooldown = max(self.manual_cooldown, 0)
            if cached and ((not refresh and cached[0] > now)
                           or (refresh and since < cooldown)):
                remaining = max(0, int(cooldown - since + .999)) if refresh else 0
                return dict(cached[1], cached=True, stale=False,
                            retry_after_seconds=remaining,
                            checked_at=self.last_checked.get(key, 0))
            self.last_attempt[key] = now
            try:
                raw, etag, headers = self._release_page_http(
                    repo, number, self.etags.get(key))
                if raw is None:
                    if not cached:
                        raise ValueError('GitHub replied 304 without a cached release page')
                    result = cached[1]
                else:
                    if not isinstance(raw, list):
                        raise ValueError('Invalid GitHub Releases response')
                    releases = []
                    for entry in raw:
                        if isinstance(entry, dict):
                            entry = dict(entry, _verified_repo=repo)
                        obj = normalise_release(entry)
                        if obj:
                            releases.append(obj)
                    # GitHub sorts by created_at, not by semantic firmware build.
                    releases.sort(key=self._sort_key, reverse=True)
                    result = {'releases': releases,
                              'has_more': len(raw) == PAGE_SIZE and number < MAX_PAGES,
                              'page': number}
                self.cache[key] = (now + self.cache_seconds, result)
                if etag:
                    self.etags[key] = etag
                self.last_checked[key] = int(time.time())
                self.failures[repo] = 0
                return dict(result, cached=raw is None, stale=False,
                            retry_after_seconds=0, checked_at=self.last_checked[key])
            except HTTPError as exc:
                if exc.code in (403, 429):
                    failures = self.failures.get(repo, 0)
                    self.failures[repo] = failures + 1
                    self.blocked_until[repo] = now + self._rate_limit_delay(exc, failures)
                else:
                    self.blocked_until[repo] = now + 120
                if cached:
                    return dict(cached[1], cached=True, stale=True,
                                retry_after_seconds=max(0, int(self.blocked_until[repo] - now + .999)),
                                checked_at=self.last_checked.get(key, 0))
                raise
            except (URLError, OSError, TimeoutError):
                self.blocked_until[repo] = now + 120
                if cached:
                    return dict(cached[1], cached=True, stale=True,
                                retry_after_seconds=120,
                                checked_at=self.last_checked.get(key, 0))
                raise

    def recent(self, repo, current=''):
        repo = valid_repo(repo)
        # GitHub orders releases by creation date; compare semantic versions so
        # a backport cannot be mistaken for a newer firmware.
        releases = []
        for page in range(1, MAX_PAGES + 1):
            result = self.page(repo, page)
            releases.extend(result['releases'])
            # Stop when five older stable versions are found, or history is exhausted.
            if not result['has_more']:
                break
            if current and version_tuple(current) and sum(not r['prerelease'] and is_older(r['tag'], current) for r in releases) >= 6:
                break
            # Many experimental build tags can fill entire GitHub pages. Do
            # not hide older Stable versions merely because 30 previews exist.
            if not current and sum(r['channel'] == 'stable' for r in releases) >= 6:
                break
        stable = [r for r in releases if r['channel'] == 'stable' and version_tuple(r['tag'])]
        stable.sort(key=lambda r: version_tuple(r['tag']), reverse=True)
        current = str(current or '').strip()
        if version_tuple(current) is None:
            current = stable[0]['tag'] if stable else ''
        older = [r for r in stable if is_older(r['tag'], current)]
        return {'current': current, 'recent': older[:5],
                'latest': stable[0] if stable else None, 'more_older': len(older) > 5,
                'source': repo, 'note': 'Vorige gepubliceerde stabiele versies; bouw altijd voor jouw boardprofiel.'}

    def channels(self, repo):
        """Newest published, immutable source release in each visible channel."""
        repo = valid_repo(repo)
        newest = {'stable': None, 'pre-release': None, 'experimental': None}
        for number in range(1, MAX_PAGES + 1):
            page = self.page(repo, number)
            for item in page['releases']:
                channel = item['channel']
                old = newest[channel]
                def key(entry):
                    return (version_tuple(entry['tag'])[:3],
                            entry['build_number'], entry['published_at'], entry['id'])
                if old is None or key(item) > key(old):
                    newest[channel] = item
            if not page['has_more']:
                break
        return {'channels': newest, 'source': repo,
                'note': 'Published source releases only. No firmware is hardware-validated automatically.'}

    def resolve(self, repo, tag, release_id):
        repo = valid_repo(repo)
        tag = clean_tag(tag)
        if version_tuple(tag) is None: raise ValueError('Only firmware Release tags may be compiled')
        if not isinstance(release_id, int) or isinstance(release_id, bool) or release_id <= 0:
            raise ValueError('Ongeldige release-ID')
        obj = self._api(repo, f'releases/tags/{quote(tag, safe="")}')
        if not isinstance(obj, dict) or obj.get('draft') or not obj.get('published_at') or obj.get('id') != release_id or obj.get('tag_name') != tag:
            raise ValueError('Release no longer exists or is unpublished')
        identity = release_identity(tag, bool(obj.get('prerelease')))
        if identity is None or (identity['channel'] != 'stable' and not obj.get('prerelease')):
            raise ValueError('Release channel and published status disagree')
        # /commits/TAG dereferences annotated tags; do not trust target_commitish
        # (it may contain a moving branch name).
        commit = self._api(repo, f'commits/{quote(tag, safe="")}', 120_000)
        sha = commit.get('sha') if isinstance(commit, dict) else None
        if not isinstance(sha, str) or not SHA.fullmatch(sha):
            raise ValueError('Release tag cannot be resolved to a commit')
        return {'repo': repo, 'tag': tag, 'release_id': release_id,
                'sha': sha, 'prerelease': bool(obj.get('prerelease')),
                **identity, 'published_at': str(obj.get('published_at'))[:32]}

    def source(self, pinned):
        # A SHA is immutable. Never allow a moving tag or mutable main branch
        # to replace this build's source after selection.
        repo = valid_repo(pinned['repo'])
        sha = pinned['sha']
        if not SHA.fullmatch(sha):
            raise ValueError('Ongeldige vastgepinde release-commit')
        return self.source_fetch(repo, sha)
