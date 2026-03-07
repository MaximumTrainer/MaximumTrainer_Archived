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
});
