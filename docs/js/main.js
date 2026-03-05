/* Maximum Trainer — Website JavaScript */
(function () {
  'use strict';

  // ── Mobile nav toggle ──────────────────────────────────────────────────────
  var toggle = document.getElementById('navToggle');
  var navLinks = document.getElementById('navLinks');

  if (toggle && navLinks) {
    toggle.addEventListener('click', function () {
      var isOpen = navLinks.classList.toggle('is-open');
      toggle.setAttribute('aria-expanded', String(isOpen));
    });

    // Close nav when a link is clicked
    navLinks.addEventListener('click', function (e) {
      if (e.target.tagName === 'A') {
        navLinks.classList.remove('is-open');
        toggle.setAttribute('aria-expanded', 'false');
      }
    });
  }

  // ── Highlight active nav link on scroll ───────────────────────────────────
  var sections = document.querySelectorAll('section[id]');
  var navAnchors = document.querySelectorAll('.nav-links a[href^="#"]');

  function onScroll() {
    var scrollY = window.scrollY || window.pageYOffset;
    var active = '';
    sections.forEach(function (section) {
      var top = section.offsetTop - 80;
      if (scrollY >= top) {
        active = section.id;
      }
    });
    navAnchors.forEach(function (a) {
      a.classList.toggle('active', a.getAttribute('href') === '#' + active);
    });
  }

  window.addEventListener('scroll', onScroll, { passive: true });
  onScroll();

  // ── Gallery lightbox ──────────────────────────────────────────────────────
  var galleryItems = document.querySelectorAll('.gallery-item img');

  if (galleryItems.length) {
    // Create overlay elements
    var overlay = document.createElement('div');
    overlay.id = 'lightbox-overlay';
    overlay.setAttribute('role', 'dialog');
    overlay.setAttribute('aria-modal', 'true');
    overlay.setAttribute('aria-label', 'Image preview');
    overlay.style.cssText = [
      'display:none',
      'position:fixed',
      'inset:0',
      'z-index:9999',
      'background:rgba(0,0,0,0.88)',
      'align-items:center',
      'justify-content:center',
      'cursor:zoom-out',
    ].join(';');

    var overlayImg = document.createElement('img');
    overlayImg.style.cssText = [
      'max-width:90vw',
      'max-height:88vh',
      'object-fit:contain',
      'border-radius:8px',
      'box-shadow:0 8px 48px rgba(0,0,0,0.7)',
      'pointer-events:none',
    ].join(';');

    overlay.appendChild(overlayImg);
    document.body.appendChild(overlay);

    function openLightbox(src, alt) {
      overlayImg.src = src;
      overlayImg.alt = alt || '';
      overlay.style.display = 'flex';
      document.body.style.overflow = 'hidden';
    }

    function closeLightbox() {
      overlay.style.display = 'none';
      overlayImg.src = '';
      document.body.style.overflow = '';
    }

    galleryItems.forEach(function (img) {
      img.addEventListener('click', function () {
        openLightbox(img.src, img.alt);
      });
      img.style.cursor = 'zoom-in';
    });

    overlay.addEventListener('click', closeLightbox);

    document.addEventListener('keydown', function (e) {
      if (e.key === 'Escape' && overlay.style.display !== 'none') {
        closeLightbox();
      }
    });
  }

})();
