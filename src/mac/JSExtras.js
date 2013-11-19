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
// Basically, due to https://bugs.webkit.org/show_bug.cgi?id=124580, we cannot
// use our cookie code to set our own cookies on the initial WebSocket
// handshakek request. So instead, we replace the system cookies with our own
// ones right before the websocket is created, and restore them right after.
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
