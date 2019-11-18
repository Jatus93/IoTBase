/*!
 * Beauter v0.3.0 (http://beauter.outboxcraft.com)
 * Copyright 2016-2018 Shubham Ramdeo, Outboxcraft
 * Licensed under MIT (https://github.com/outboxcraft/beauter/blob/master/LICENSE)
 */
function showsnackbar(a){var b=document.getElementById(a);b.className=b.className.replace("snackbar","snackbar show");setTimeout(function(){b.className=b.className.replace("show","")},3E3)}function topnav(a){a=document.getElementById(a);a.classList.contains("responsive")?a.className=a.className.replace("responsive",""):a.className+=" responsive"}function openmodal(a){var b=document.getElementById(a);b.style.display="block";window.onclick=function(a){a.target==b&&(b.style.display="none")}}
function openimg(a,b){var c=document.getElementById(a),d=document.getElementById(b),e=d.getElementsByClassName("modal-content")[0],f=d.getElementsByClassName("caption")[0];d.style.display="block";e.src=c.src;f.innerHTML=c.alt}var close=document.getElementsByClassName("-close"),i;
for(i=0;i<close.length;i++)close[i].onclick=function(){var a=this.parentElement;a.classList.contains("modalbox-modal-content")&&(a=a.parentElement);a.style.opacity="0";setTimeout(function(){a.style.display="none";a.style.opacity="1"},600)};var accr=document.getElementsByClassName("accordion"),j;for(j=0;j<accr.length;j++)accr[j].onclick=function(){this.classList.toggle("active");var a=this.nextElementSibling;a.style.maxHeight=a.style.maxHeight?null:a.scrollHeight+"px"};
function opentab(a,b){var c;var d=document.getElementsByClassName("tabcontent");for(c=0;c<d.length;c++)d[c].style.display="none";d=document.getElementsByClassName("tablinks");for(c=0;c<d.length;c++)d[c].className=d[c].className.replace(" active","");document.getElementById(a).style.display="block";b.currentTarget.className+=" active"};

/*!
 * JS FOR IOT BASE 
 */


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
    document.getElementById(elementID).classList.remove('_hidden');
}

function hide(elementID){
    document.getElementById(elementID).classList.add('_hidden');
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
    openmodal('loading');
}

function resetCommand(){
    wsocket.send('{"reset":1}');
    openmodal('loading');
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
    openmodal('loading');
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
