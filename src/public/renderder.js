// Tauri IPC adapter - replaces Electron's ipcRenderer
// Uses dynamic import of @tauri-apps/api

(function() {
    'use strict';

    // Path utilities (replaces Node's path module)
    window.pathUtil = {
        join: function() {
            var args = Array.prototype.slice.call(arguments);
            var parts = [];
            for (var i = 0; i < args.length; i++) {
                if (typeof args[i] !== 'string') continue;
                var p = args[i].replace(/[\\/]+$/, '');
                if (p) parts.push(p);
            }
            return parts.join('\\');
        },
        dirname: function(p) {
            if (!p) return '.';
            var lastSep = Math.max(p.lastIndexOf('\\'), p.lastIndexOf('/'));
            if (lastSep === -1) return '.';
            return p.substring(0, lastSep);
        },
        extname: function(p) {
            if (!p) return '';
            var lastSep = Math.max(p.lastIndexOf('\\'), p.lastIndexOf('/'));
            var lastDot = p.lastIndexOf('.');
            if (lastDot <= lastSep) return '';
            return p.substring(lastDot).toLowerCase();
        }
    };

    // Backend communication - replaces sendMessageToMain / ipcRenderer
    window.sendToBackend = function(cmd, args) {
        var ta = window.__TAURI__;
        if (ta) { ta = ta.core || ta; }
        var invoke = ta && ta.invoke;
        if (invoke) {
            return invoke(cmd, args || {});
        }
        return Promise.reject('Tauri API 不可用');
    };
})();
