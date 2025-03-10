import Module from "./hello_world_main.js";

async function handleRequest(request) {
  let output = "";
  const m = await Module({
    print: s => output = s,
    instantiateWasm: (imports, callback) => {
     const instance = new WebAssembly.Instance(WASM_MAIN, imports);
      callback(instance);
      return instance.exports;
    }
  });

  const user = new URL(request.url).searchParams.get("user") || "";
  m.callMain([user]);
  return new Response(output);
}

addEventListener("fetch", event => {
	event.respondWith(handleRequest(event.request));
});
