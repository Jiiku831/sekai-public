import Module from "./server_main.js";

var module = null;
var moduleReady = initModule();
async function initModule() {
  module = await Module({
    instantiateWasm: (imports, callback) => {
     const instance = new WebAssembly.Instance(WASM_MAIN, imports);
      callback(instance);
      return instance.exports;
    },
  });
  console.log("WASM Module Initialized");
  return true;
}

function stringToNewUTF8(str) {
  let bufSize = module.lengthBytesUTF8(str) + 1;
  let ptr = module._Malloc(bufSize);
  module.stringToUTF8(str, ptr, bufSize);
  return ptr;
}

function callHandler(path, input) {
  const inputPtr = stringToNewUTF8(input);
  const pathPtr = stringToNewUTF8(path);
  const resultPtr = module._HandleRequest(pathPtr, inputPtr);
  const code = module.getValue(resultPtr, "i8");
  const result = module.UTF8ToString(resultPtr + 1);
  module._Free(inputPtr);
  module._Free(pathPtr);
  module._Free(resultPtr);
  return [code, result];
}

function mapCode(code) {
  return {
    0: 200,   // OK
    1: 500,   // Cancelled
    2: 500,   // Unknown
    3: 400,   // Invalid Argument
    4: 500,   // Deadline Exceeded
    5: 404,   // Not Found
    6: 500,   // Already Exists
    7: 403,   // Permission Denied
    8: 500,   // Resource Exhausted
    9: 500,   // Failed Precondition
    10: 500,  // Aborted
    11: 500,  // Out of Range
    12: 501,  // Unimplemented
    13: 500,  // Internal
    14: 503,  // Unavailable
    15: 500,  // Data Loss
    16: 401,  // Unauthenticated
  }[code] || 500;
}

async function handlePreflightRequest(request) {
  let headers = new Headers();
  headers.append("Access-Control-Allow-Origin", "*");
  headers.append("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  headers.append("Access-Control-Max-Age", "86400");
  return new Response(null, {
    status: 204,
    headers: headers
  });
}

async function handleRequest(request) {
  if (request.method == "OPTIONS") {
    return handlePreflightRequest(request);
  }
  await moduleReady;
  let output = "";
  const url = new URL(request.url);
  let body = request.text();
  let [code, result] = callHandler(url.pathname, await body);
  let headers = new Headers();
  headers.append("Access-Control-Allow-Origin", "*");
  headers.append("Content-Type", "application/json; charset=UTF-8");
  return new Response(result, {
    status: mapCode(code),
    headers: headers
  });
}

addEventListener("fetch", event => {
  event.respondWith(handleRequest(event.request));
});
