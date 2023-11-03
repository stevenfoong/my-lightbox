// save some bytes
const gel = (e) => document.getElementById(e);

const wifi_div = gel("wifi");
const connect_div = gel("connect");
const connect_manual_div = gel("connect_manual");
const connect_wait_div = gel("connect-wait");
const connect_details_div = gel("connect-details");

function docReady(fn) {
  // see if DOM is already available
  if (
    document.readyState === "complete" ||
    document.readyState === "interactive"
  ) {
    // call on next available tick
    setTimeout(fn, 1);
  } else {
    document.addEventListener("DOMContentLoaded", fn);
  }
}


docReady(async function () {

  gel("wifi_save").addEventListener(
    "click",
    (e) => {
      wifi_save();
    },
    false
  );

  gel("restart_unit").addEventListener(
    "click",
    (e) => {
      unit_restart();
    },
    false
  );

});

async function wifi_save() {

  var pwd;
  //if (conntype == "wifi_save") {
    //Grab the manual SSID and PWD
  selectedSSID = gel("wifi_ssid").value;
  pwd = gel("wifi_pwd").value;
  //} else {
  //  pwd = gel("pwd").value;
  //}
  //reset connection
  //gel("loading").style.display = "block";
  //gel("connect-success").style.display = "none";
  //gel("connect-fail").style.display = "none";

  //gel("ok-connect").disabled = true;
  //gel("ssid-wait").textContent = selectedSSID;
  //connect_div.style.display = "none";
  //connect_manual_div.style.display = "none";
  //connect_wait_div.style.display = "block";

  await fetch("save_wifi.json", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "X-Custom-ssid": selectedSSID,
      "X-Custom-pwd": pwd,
    },
    body: { timestamp: Date.now() },
  });

}

async function unit_restart() {

  await fetch("/restart_unit", {
    method: "GET",
  });

}

async function performConnect(conntype) {

  var pwd;
  //if (conntype == "wifi_save") {
    //Grab the manual SSID and PWD
  selectedSSID = gel("wifi_ssid").value;
  pwd = gel("wifi_pwd").value;
  //} else {
  //  pwd = gel("pwd").value;
  //}
  //reset connection
  //gel("loading").style.display = "block";
  //gel("connect-success").style.display = "none";
  //gel("connect-fail").style.display = "none";

  //gel("ok-connect").disabled = true;
  //gel("ssid-wait").textContent = selectedSSID;
  //connect_div.style.display = "none";
  //connect_manual_div.style.display = "none";
  //connect_wait_div.style.display = "block";

  await fetch("connect.json", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "X-Custom-ssid": selectedSSID,
      "X-Custom-pwd": pwd,
    },
    body: { timestamp: Date.now() },
  });

}

function rssiToIcon(rssi) {
  if (rssi >= -60) {
    return "w0";
  } else if (rssi >= -67) {
    return "w1";
  } else if (rssi >= -75) {
    return "w2";
  } else {
    return "w3";
  }
}


function refreshAPHTML(data) {
  var h = "";
  data.forEach(function (e, idx, array) {
    let ap_class = idx === array.length - 1 ? "" : " brdb";
    let rssicon = rssiToIcon(e.rssi);
    let auth = e.auth == 0 ? "" : "pw";
    h += `<div class="ape${ap_class}"><div class="${rssicon}"><div class="${auth}">${e.ssid}</div></div></div>\n`;
  });

  gel("wifi-list").innerHTML = h;
}

