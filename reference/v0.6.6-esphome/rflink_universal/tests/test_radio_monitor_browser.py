"""Chromium end-to-end UI regression against a fake local HTTP/API server.

Run only on development machines with Playwright + Chromium; these are NOT
requirements for Windows MAAK_RELEASE.bat or the ESPHome device.
"""
from pathlib import Path
import json
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
html = (ROOT / 'ui' / 'index.html').read_text(encoding='utf-8').replace('/*__RFLINK_LOCALES__*/', (ROOT / 'ui' / 'locales.bundle.json').read_text(encoding='utf-8'))
state = {'online': True, 'missing': {0, 1, 2}, 'events': []}

def status():
    return {
        'uptime_ms': 100000, 'total_frames': 2, 'decoded_frames': 0,
        'unknown_frames': 2, 'led': {'status': 'gereed', 'brightness': 20, 'quiet': False},
        'channels': [{
            'index': i, 'name': name, 'frequency_label': label,
            'available': i not in state['missing'], 'checked': True,
            'rx_ready': i not in state['missing'], 'tx_ready': i not in state['missing'],
            'frames': 1, 'age_ms': 300, 'rssi_dbm': None,
            'mode': 'PACKET' if i == 2 else 'RX'
        } for i, name, label in [(0, '433.42', '433.42 MHz'), (1, '433.92', '433.92 MHz'),
                                 (2, '868.30 Itho', '868.30 MHz')]],
        'activity': state['events']
    }

# A browser-rendered DOM test does not need external HTTP access.
script = html.split('<script>', 1)[1].split('</script>', 1)[0]
static_html = html.replace('<script>' + script + '</script>', '')

def push_state(page):
    page.evaluate('status => { window.__mockStatus = status; }', status())

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True, executable_path='/usr/bin/chromium',
                                args=['--no-sandbox', '--disable-dev-shm-usage'])
    page = browser.new_page()
    errors = []
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.set_content(static_html)
    page.evaluate("""() => {
      window.__mockStatus = null;
      window.__mockOnline = true;
      window.fetch = async (url, opt) => {
        if (url === '/rflink/api/status' && !window.__mockOnline)
          return {ok:false, text:async ()=>'offline'};
        let result;
        if (url === '/rflink/api/language') result={language:'nl'};
        else if (url === '/rflink/api/status') result=window.__mockStatus;
        else if (url === '/rflink/api/activity-export') result={activity:(window.__mockStatus&&window.__mockStatus.activity)||[]};
        else if (url === '/rflink/api/devices') result={devices:[],candidates:[]};
        else if (url === '/rflink/api/decoders') result={decoders:[]};
        else if (url.startsWith('/rflink/api/developer')) result={channel:0,mode:'listen',best:null};
        else throw new Error('Unexpected request: '+url);
        return {ok:true,json:async()=>result};
      };
    }""")
    push_state(page)
    page.evaluate(script)
    page.wait_for_function("document.documentElement.lang === 'nl'")
    page.wait_for_timeout(120)
    page.locator('.side button[data-page="learn"]').click()
    assert page.locator('#learnRadios button[disabled]').count() == 3
    assert page.locator('#baselineBtn').is_disabled()
    assert page.locator('#listenBtn').is_disabled()
    page.locator('.side button[data-page="monitor"]').click()
    assert page.locator('#monitorRadios button').count() == 4
    assert page.locator('#monitorRadios button[disabled]').count() == 0  # historical logs selectable offline
    assert 'Geen geregistreerde activiteit' in page.locator('#monitorActivity').inner_text()
    print('Browser: 3 absent radios greyed out; learning/monitor disabled, all-radio overview available: PASS')

    state['online'] = True
    page.evaluate('window.__mockOnline=true')
    state['missing'] = set()
    state['events'] = [
        {'channel_index': 0, 'channel': '433.42', 'direction': 'RX', 'protocol': 'Onbekend',
         'summary': '433 signal', 'detail': 'RAW hash ABCD | 27 pulsen', 'age_ms': 600},
        {'channel_index': 1, 'channel': '433.92', 'direction': 'RX', 'protocol': 'Onbekend',
         'summary': '433.92 RX only', 'detail': 'RAW hash 1234 | 38 pulsen', 'age_ms': 550},
        {'channel_index': 2, 'channel': '868.30 Itho', 'direction': 'RX', 'protocol': 'Itho',
         'summary': '868 RX', 'detail': 'AB FE ... (63 bytes)', 'age_ms': 500},
        {'channel_index': 2, 'channel': '868.30 Itho', 'direction': 'TX', 'protocol': 'TX-opdracht',
         'summary': '868 TX aangevraagd', 'detail': '<script>alert(1)</script>', 'age_ms': 300},
        {'channel_index': 2, 'channel': '868.30 Itho', 'direction': 'RX', 'protocol': 'Onbekend',
         'summary': '868 suspected noise', 'detail': '63 bytes random', 'age_ms': 250,
         'noise_suspect': True, 'noise_reason': '63-byte packet does not match the active Itho profile'}
    ]
    push_state(page)
    page.wait_for_timeout(3500)  # normal 3-second status refresh
    assert page.locator('#monitorRadios button[disabled]').count() == 0
    assert '433 signal' in page.locator('#monitorActivity').inner_text()
    # Every radio must be independently selectable, not just the first and 868.
    for index, summary in [(0, '433 signal'), (1, '433.92 RX only'), (2, '868 RX')]:
        page.locator(f'#monitorRadios button[data-mon="{index}"]').click()
        text = page.locator('#monitorActivity').inner_text()
        assert summary in text, (index, text)
        for other in ('433 signal', '433.92 RX only', '868 RX'):
            if other != summary:
                assert other not in text, (index, other, text)
    assert '868 RX' in page.locator('#monitorActivity').inner_text()
    assert '868 TX aangevraagd' in page.locator('#monitorActivity').inner_text()
    assert page.locator('#monitorActivity .card.notice').count() == 0
    assert '433 signal' not in page.locator('#monitorActivity').inner_text()
    page.locator('.monitor-advanced summary').click()
    page.locator('#monitorDirection').select_option('TX')
    assert '868 RX' not in page.locator('#monitorActivity').inner_text()
    assert '868 TX aangevraagd' in page.locator('#monitorActivity').inner_text()
    page.locator('#monitorDirection').select_option('RX')
    assert page.locator('#monitorActivity .card.notice').count() == 0
    page.locator('#monitorLogType').select_option('noise')
    assert page.locator('#monitorActivity .card.notice').count() == 1
    assert page.locator('#monitorActivity script').count() == 0
    print('Browser: all 3 radios selectable; channel isolation, RX/TX filtering, noise toggle and escaped HTML: PASS')

    page.locator('.side button[data-page="learn"]').click()
    assert page.locator('#learnRadios button[disabled]').count() == 0
    page.locator('#learnRadios button[data-ch="2"]').click()
    assert 'active' in page.locator('#learnRadios button[data-ch="2"]').get_attribute('class')
    assert page.locator('#baselineBtn').is_enabled()
    state['missing'] = {2}
    push_state(page)
    page.wait_for_timeout(3500)
    assert page.locator('#learnRadios button[data-ch="2"]').is_disabled()
    assert 'active' not in page.locator('#learnRadios button[data-ch="2"]').get_attribute('class')
    state['missing'] = {0, 1, 2}
    push_state(page)
    page.wait_for_timeout(3500)
    assert page.locator('#baselineBtn').is_disabled()
    assert page.locator('#listenBtn').is_disabled()
    assert not errors, errors
    print('Browser: all three individual channels, 868MHz learn selection, status refresh and disable: PASS')
    page.set_viewport_size({'width': 390, 'height': 844})
    assert page.locator('.bottom button[data-page="monitor"]').is_visible()
    assert page.locator('.bottom button').count() == 5
    page.locator('.bottom button[data-page="monitor"]').click()
    assert page.locator('[data-view="monitor"]').is_visible()
    assert page.locator('#monitorRadios button[disabled]').count() == 0  # historical logs selectable offline
    assert not errors, errors
    print('Browser: responsive mobile navigation with 5 tabs and missing-radio status: PASS')
    browser.close()
