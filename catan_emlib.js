// Override Emscripten's default fd_write so that stdout/stderr bytes are
// streamed straight to Module.jsWrite without per-line buffering. The
// default implementation only flushes on '\n' or NUL, which means terminal
// prompts (e.g. "Your choice: ") never appear, and the newline byte itself
// is consumed rather than forwarded — so all lines collapse onto one row.
//
// Built into the wasm bundle via:  emcc --js-library catan_emlib.js ...

mergeInto(LibraryManager.library, {
  fd_write: function (fd, iov, iovcnt, pnum) {
    var num = 0;
    var chunks = [];
    for (var i = 0; i < iovcnt; i++) {
      var ptr = HEAPU32[iov >> 2];
      var len = HEAPU32[(iov + 4) >> 2];
      iov += 8;
      if (len > 0) chunks.push(HEAPU8.subarray(ptr, ptr + len));
      num += len;
    }
    HEAPU32[pnum >> 2] = num;
    if (chunks.length === 0) return 0;
    // concat
    var total = 0;
    for (var j = 0; j < chunks.length; j++) total += chunks[j].length;
    var merged = new Uint8Array(total);
    var off = 0;
    for (var k = 0; k < chunks.length; k++) {
      merged.set(chunks[k], off);
      off += chunks[k].length;
    }
    var s = (typeof TextDecoder !== 'undefined')
      ? new TextDecoder('utf-8').decode(merged)
      : Array.from(merged).map(function (c) { return String.fromCharCode(c); }).join('');
    if (typeof Module.jsWrite === 'function') {
      Module.jsWrite(s);
    } else {
      // Fall back to console for fd 2 (stderr); avoids losing crash diagnostics.
      (fd === 2 ? console.error : console.log)(s);
    }
    return 0;
  },
});
