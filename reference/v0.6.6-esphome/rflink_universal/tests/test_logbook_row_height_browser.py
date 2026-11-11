"""LOGROWFIX1 regression: logbook entries must not expand to fill the viewport.

Uses the real RFLink UI stylesheet and representative event markup. Both summary
rows and the content area are rendered by Chromium to exercise CSS Grid sizing.
"""
from pathlib import Path
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
html = (ROOT / 'ui' / 'index.html').read_text(encoding='utf-8')
style = html.split('<style>', 1)[1].split('</style>', 1)[0]

with sync_playwright() as pw:
    browser = pw.chromium.launch(headless=True, executable_path='/usr/bin/chromium', args=['--no-sandbox', '--disable-dev-shm-usage'])
    for width in [1440, 390]:
        page = browser.new_page(viewport={'width': width, 'height': 900})
        page.set_content(f'<style>{style}</style><div id="logbookTimeline" class="list" style="gap:4px"></div>')
        row_markup = '''<details class="logbook-entry">
          <summary><span class="mono muted small">0.38s</span><b>⚙ Systeem</b>
          <span class="logbook-severity lv-info">Info</span><span class="logbook-message">RFLink gestart; system trace records decisions</span></summary>
          <div>Uitgebreide systeemdetails met identificatie en diagnose.</div>
        </details>'''
        baseline_height = None
        for count in [1, 2, 5, 30]:
            page.locator('#logbookTimeline').evaluate('(el, args) => el.innerHTML = args.html.repeat(args.count)', {'html': row_markup, 'count': count})
            state = page.evaluate('''() => {
                const panel = document.querySelector('#logbookTimeline');
                const rows = [...panel.querySelectorAll('.logbook-entry')];
                return {
                  height: panel.getBoundingClientRect().height,
                  clientHeight: panel.clientHeight,
                  scrollHeight: panel.scrollHeight,
                  rowHeights: rows.map(el => el.getBoundingClientRect().height),
                  lastBottom: rows.at(-1).getBoundingClientRect().bottom,
                  panelBottom: panel.getBoundingClientRect().bottom,
                  alignContent: getComputedStyle(panel).alignContent,
                  gridAutoRows: getComputedStyle(panel).gridAutoRows
                };
            }''')
            if baseline_height is None:
                baseline_height = state['height']
            assert abs(state['height'] - baseline_height) < 1, (width, count, state)
            assert state['alignContent'] == 'start' and state['gridAutoRows'] == 'max-content', (width, count, state)
            assert all(35 <= h <= 75 for h in state['rowHeights']), (width, count, state)
            if count <= 5:
                assert state['scrollHeight'] <= state['clientHeight'], (width, count, state)
                assert state['lastBottom'] < state['panelBottom'] - 25, (width, count, state)
            else:
                assert state['scrollHeight'] > state['clientHeight'], (width, count, state)
            print(f'PASS {width}px {count} entries: height {state["height"]:.0f}px, rows {state["rowHeights"][0]:.1f}px, scroll {state["scrollHeight"] > state["clientHeight"]}', flush=True)
        page.locator('.logbook-entry').first.locator('summary').click()
        assert page.locator('.logbook-entry').first.evaluate('(e) => e.open')
        first_height = page.locator('.logbook-entry').first.evaluate('(e) => e.getBoundingClientRect().height')
        assert first_height > 75, (width, first_height)
        # With a collapsed list, all entries remain compact and do not stretch on refresh.
        page.locator('#logbookTimeline').evaluate('(el, html) => el.innerHTML = html.repeat(2)', row_markup)
        assert max(page.locator('.logbook-entry').evaluate_all('(els) => els.map(e => e.getBoundingClientRect().height)')) < 75
        page.close()
    browser.close()
print('LOGROWFIX1 responsive geometry and expanded-details tests: PASS', flush=True)
