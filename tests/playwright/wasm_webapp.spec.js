// @ts-check
const { test, expect, request } = require('@playwright/test');

const APP_URL = 'https://maximumtrainer.github.io/MaximumTrainer_Redux/app/';
const BASE_URL = 'https://maximumtrainer.github.io/MaximumTrainer_Redux/app';

// ── HTTP asset checks ──────────────────────────────────────────────────────
test.describe('WASM assets are deployed', () => {
  test('qtloader.js returns 200', async ({ request }) => {
    const res = await request.get(`${BASE_URL}/qtloader.js`);
    expect(res.status(), 'qtloader.js should be deployed (200), not missing (404)').toBe(200);
  });

  test('MaximumTrainer.js returns 200', async ({ request }) => {
    const res = await request.get(`${BASE_URL}/MaximumTrainer.js`);
    expect(res.status(), 'MaximumTrainer.js should be deployed (200), not missing (404)').toBe(200);
  });

  test('MaximumTrainer.wasm returns 200', async ({ request }) => {
    const res = await request.get(`${BASE_URL}/MaximumTrainer.wasm`);
    expect(res.status(), 'MaximumTrainer.wasm should be deployed (200), not missing (404)').toBe(200);
  });

  test('logger.js returns 200', async ({ request }) => {
    const res = await request.get(`${BASE_URL}/logger.js`);
    expect(res.status(), 'logger.js should be deployed (200), not missing (404)').toBe(200);
  });
});

// ── Page-level checks ──────────────────────────────────────────────────────
test.describe('WASM webapp page', () => {
  test('"not deployed" message is absent', async ({ page }) => {
    // Inject a stub navigator.bluetooth so the page proceeds past the
    // browser-capability guard and attempts to load the WASM assets.
    await page.addInitScript(() => {
      if (!navigator.bluetooth) {
        Object.defineProperty(navigator, 'bluetooth', {
          value: { requestDevice: async () => { throw new Error('stub'); } },
          configurable: true,
        });
      }
    });

    const errors = [];
    page.on('pageerror', err => errors.push(err.message));

    // Collect network failures for WASM asset URLs
    const failedRequests = [];
    page.on('requestfailed', req => {
      if (req.url().includes('qtloader') || req.url().includes('MaximumTrainer')) {
        failedRequests.push(req.url());
      }
    });

    await page.goto(APP_URL, { waitUntil: 'domcontentloaded' });

    // Give the JS time to attempt loading qtloader.js and react to success/failure
    await page.waitForTimeout(4000);

    // The "not deployed" sentinel text must NOT appear
    const notDeployedText = await page.locator('text=WebAssembly build hasn\'t been deployed yet').count();
    expect(notDeployedText, 'The "not deployed" fallback message should not be visible').toBe(0);

    // No WASM asset fetches should have failed
    expect(failedRequests, `These WASM assets failed to load: ${failedRequests.join(', ')}`).toHaveLength(0);
  });

  test('loading screen or Qt canvas is present', async ({ page }) => {
    await page.addInitScript(() => {
      if (!navigator.bluetooth) {
        Object.defineProperty(navigator, 'bluetooth', {
          value: { requestDevice: async () => { throw new Error('stub'); } },
          configurable: true,
        });
      }
    });

    await page.goto(APP_URL, { waitUntil: 'domcontentloaded' });
    await page.waitForTimeout(3000);

    // Either the loading screen (progress bar) or the Qt canvas wrapper must be visible
    const loadingScreen = page.locator('#loading-screen');
    const canvasWrapper = page.locator('#qt-canvas-wrapper');

    const loadingVisible = await loadingScreen.isVisible();
    const canvasVisible  = await canvasWrapper.evaluate(el => el.style.visibility !== 'hidden');

    expect(loadingVisible || canvasVisible,
      'Either the loading screen or the Qt canvas should be visible').toBe(true);
  });

  test('WASM log overlay is present and has the copy button', async ({ page }) => {
    await page.addInitScript(() => {
      if (!navigator.bluetooth) {
        Object.defineProperty(navigator, 'bluetooth', {
          value: { requestDevice: async () => { throw new Error('stub'); } },
          configurable: true,
        });
      }
    });

    await page.goto(APP_URL, { waitUntil: 'domcontentloaded' });

    // Wait for the overlay to appear in the DOM (injected by logger.js on DOMContentLoaded)
    await page.waitForSelector('#wasm-log-overlay', { timeout: 10000 });

    // The log overlay must exist in the DOM
    const overlay = page.locator('#wasm-log-overlay');
    expect(await overlay.count(), '#wasm-log-overlay should be present in the DOM').toBe(1);

    // The copy button must exist inside the overlay
    const copyBtn = page.locator('#wasm-log-copy-btn');
    expect(await copyBtn.count(), '#wasm-log-copy-btn should be present inside the overlay').toBe(1);

    // Wait for at least one log line to be written, then verify
    await page.waitForFunction(() => {
      const overlay = document.querySelector('#wasm-log-overlay');
      return overlay && overlay.querySelectorAll('div > div').length > 0;
    }, { timeout: 10000 });
    const logLines = overlay.locator('div > div');
    const lineCount = await logLines.count();
    expect(lineCount, 'The log overlay should contain at least one log entry').toBeGreaterThan(0);
  });
});
