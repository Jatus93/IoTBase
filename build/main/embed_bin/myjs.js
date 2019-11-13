var theUrl = "http://"+window.location.hostname;
var nets = JSON.parse("{}");
var wsocket = new WebSocket("ws://"+window.location.hostname);
var cWait = false;

function sleep(milliseconds) {
    var start = new Date().getTime();
    for (var i = 0; i < 1e7; i++) {
        if ((new Date().getTime() - start) > milliseconds){
            break;
        }
    }
}

function setUpWs(){
    wsocket.onopen = function(evt){wsocket.send("It's open! Hooray!!!")};
    wsocket.onclose = function(evt){console.log("web socket error");setTimeout(function(){wsocket = new WebSocket("ws://"+window.location.hostname);},5000)};
    wsocket.onmessage = function(evt){console.log(evt.data);checkResponse(evt.data)};
    wsocket.onerror = function(evt){window.alert("error "+evt.data)};
}
function setVisible(elementID) {
    document.getElementById(elementID).classList.remove('hidden_block');
    document.getElementById(elementID).classList.add('visible_block');
}

function hide(elementID){
    document.getElementById(elementID).classList.remove('visible_block');
    document.getElementById(elementID).classList.add('hidden_block');
}

function checkResponse(e){
    nets = JSON.parse(e);
    if(cWait){
        console.log("verify connection");
        if(nets['ok'])
        successConnection();
        else{
            cWait = false;
            failConnection();
        }
    }
    else
    setContent(nets);
    
}
function setContent(Res) {
    var keys = Object.keys(Res);
    var count = keys.length;
    var result = "";
    keys.forEach(key => {
        result += "<tr><td>";
        result += Res[key]['ssid'];
        result +="</td><td>";
        result += Res[key]['signal'];
        result += "</td><td>";
        result += Res[key]['security'];
        result += "</td>";
        result += "<td><button class=\"btn btn-sm btn-a\" onclick=\"setValues("+ key +");hide('mainList');setVisible('input')\">connect</button></td></tr>";
    });
    document.getElementById("tbody").innerHTML = result;
}

function setValues (id) {
    var Res = nets[id];
    document.getElementById('ssid').value = Res['ssid'];
    document.getElementById('security').selectedIndex = Res['nSec'];
    document.getElementById('channel').value = Res['channel'];
}

function tryConnect(){
    var form = document.getElementsByTagName('form');
    var request = {};
    request['connect'] = 1;
    request['ssid'] = document.getElementById('ssid').value;
    request['pwd'] =  document.getElementById('pwd').value;
    request['sec'] = document.getElementById('security').value;
    request['channel'] = document.getElementById('channel').value;
    var r = JSON.stringify(request);
    wsocket.send(r);
    cWait = true;
    setVisible('loading');
}

function refreshList(){
    var request = {};
    request['refresh'] = 1;
    var r = JSON.stringify(request);
    wsocket.send(r);
}

function wpsCommand(){
    wsocket.send('{"wps":1}');
}

function sendSaveCommand(){
    var form = document.getElementsByTagName('form');
    var request = {};
    request['save'] = 1;
    request['ssid'] = document.getElementById('ssid').value;
    request['pwd'] =  document.getElementById('pwd').value;
    request['sec'] = document.getElementById('security').value;
    request['channel'] = document.getElementById('channel').value;
    var r = JSON.stringify(request);
    wsocket.send(r);
    cWait = true;
    setVisible('loading');
}

function successConnection(){
    hide('loading');
    cssFade('success');
    setVisible('save');
}
function failConnection(){
    hide('loading');
    cssFade('fail');
}
function cssFade(elementID){
    var x = document.getElementById(elementID);
    x.classList.add("show");
    setTimeout(function(){ x.classList.remove('show'); }, 3000);
}
window.addEventListener("load",setUpWs,false);