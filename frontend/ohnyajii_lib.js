var controller = null;

function InitPage () {
  controller = new Module.OhNyaJiiController();
  document.getElementById("controls").style.display = "block";
  document.getElementById("static-warning").style.display = "none";
  console.log("Ready.");
}

function OnDateChange () {
  const date = document.getElementById("date");
  if (!date.validity.valid && date.value != "") {
    return;
  }
  controller.RefreshMaxCr(date.value);
}

function OnDateClear () {
  const date = document.getElementById("date");
  date.value = "";
  controller.RefreshMaxCr("");
}
