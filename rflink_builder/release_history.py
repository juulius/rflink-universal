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


class ReleaseHistory:
    def __init__(self, fetch=read_http, source_fetch=github_firmware_zip, cache_seconds=90):
        self.fetch = fetch
        self.source_fetch = source_fetch
        self.cache_seconds = cache_seconds
        self.cache = {}
        self.lock = threading.Lock()

    def _api(self, repo, suffix, limit=1_250_000):
        repo = valid_repo(repo)
        return json.loads(self.fetch(f'https://api.github.com/repos/{repo}/{suffix}', limit))

    def page(self, repo, number=1):
        repo = valid_repo(repo)
        if not isinstance(number, int) or not 1 <= number <= MAX_PAGES:
            raise ValueError('Ongeldige pagina (1–20)')
        key = (repo, number)
        with self.lock:
            cached = self.cache.get(key)
            if cached and cached[0] > time.monotonic():
                return cached[1]
        # Only the public, published Releases API is used (not Git tags alone).
        raw = self._api(repo, f'releases?per_page={PAGE_SIZE}&page={number}')
        if not isinstance(raw, list):
            raise ValueError('Invalid GitHub Releases response')
        result = []
        for entry in raw:
            if isinstance(entry, dict):
                entry = dict(entry, _verified_repo=repo)
            obj = normalise_release(entry)
            if obj:
                result.append(obj)
        page = {'releases': result, 'has_more': len(raw) == PAGE_SIZE and number < MAX_PAGES,
                'page': number}
        with self.lock:
            self.cache[key] = (time.monotonic() + self.cache_seconds, page)
        return page

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
            if not current and len(releases) >= 30:
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
