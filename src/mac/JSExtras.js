//
// This file is injected into the Zulip website at runtime by
// HWebView_mac.mm
//

// The built-in WebKit sendAsBinary crashes when we try to use it with a large
// payload---specifically, when pasting large images. The traceback is deep in
// array handling code in JSC.
//
// To avoid the crash, we replace the native sendAsBinary with the suggested polyfill
// using typed arrays, from https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest#sendAsBinary()
XMLHttpRequest.prototype.sendAsBinary = function (sData) {
     var nBytes = sData.length, ui8Data = new Uint8Array(nBytes);
     for (var nIdx = 0; nIdx < nBytes; nIdx++) {
         ui8Data[nIdx] = sData.charCodeAt(nIdx) & 0xff;
     }
     this.send(ui8Data.buffer);
   }

// Hook into websocket creation to swizzle the system cookies
document.addEventListener('DOMContentLoaded', function(event) {
 $(document).on('websocket_preopen.zulip', function (event) {
     if (typeof window.bridge !== 'undefined' &&
         typeof window.bridge.websocketPreOpen !== 'undefined') {
         window.bridge.websocketPreOpen();
     }
 });
 $(document).on('websocket_postopen.zulip', function (event) {
     if (typeof window.bridge !== 'undefined' &&
         typeof window.bridge.websocketPostOpen !== 'undefined') {
         window.bridge.websocketPostOpen();
     }
 });
});
