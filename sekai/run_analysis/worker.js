import Module from "./server_main.js";

function stringToNewUTF8(module, str) {
  let bufSize = module.lengthBytesUTF8(str) + 1;
  let ptr = module._Malloc(bufSize);
  module.stringToUTF8(str, ptr, bufSize);
  return ptr;
}

function callHandler(module, path, input) {
  const inputPtr = stringToNewUTF8(module, input);
  const pathPtr = stringToNewUTF8(module, path);
  const resultPtr = module._HandleRequest(pathPtr, inputPtr);
  const code = module.getValue(resultPtr, "i8");
  const result = module.UTF8ToString(resultPtr + 1);
  module._Free(inputPtr);
  module._Free(pathPtr);
  module._Free(resultPtr);
  return [code, result];
}

var m = null;

async function getModule() {
  if (m != null) {
    return m;
  }
  m = await Module({
    instantiateWasm: (imports, callback) => {
     const instance = new WebAssembly.Instance(WASM_MAIN, imports);
      callback(instance);
      return instance.exports;
    },
  });
  return m;
}

function mapCode(code) {
  return {
    // OK
    0: 200,
    // Cancelled
    1: 500,
    // Unknown
    2: 500,
    // Invalid Argument
    3: 400,
    // Deadline Exceeded
    4: 500,
    // Not Found
    5: 404,
    // Already Exists
    6: 500,
    // Permission Denied
    7: 403,
    // Resource Exhausted
    8: 500,
    // Failed Precondition
    9: 500,
    // Aborted
    10: 500,
    // Out of Range
    11: 500,
    // Unimplemented
    12: 501,
    // Internal
    13: 500,
    // Unavailable
    14: 503,
    // Data Loss
    15: 500,
    // Unauthenticated
    16: 401,
  }[code] || 500;
}

async function handleRequest(request) {
  let output = "";

  const url = new URL(request.url);
  let module = getModule();
  let body = request.text();
  let [code, result] = callHandler(await module, url.pathname, await body);
  return new Response(result, {
    status: mapCode(code),
  });
}

addEventListener("fetch", event => {
	event.respondWith(handleRequest(event.request));
});
