// ---------------------------------------------------------------------------
//  WebPage.h - the control page, served from flash. Plain HTML/JS, no
//  external assets: it has to work on a LAN with no internet.
//  Everything dynamic goes through textContent, never innerHTML.
// ---------------------------------------------------------------------------
#pragma once

#include <pgmspace.h>

static const char kWebPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Desk Buddy</title>
<style>
:root{--bg:#0a0c0f;--card:#12161b;--edge:#262d35;--ink:#e4e9ee;--mute:#8b98a5;--acc:#3fd3dd;--acc-ink:#06262a}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.5 -apple-system,system-ui,"Segoe UI",Roboto,sans-serif}
.wrap{max-width:640px;margin:0 auto;padding:20px 16px 48px}
h1{font-size:1.5rem;margin:0 0 2px;letter-spacing:-.01em}
.sub{color:var(--mute);font-size:.85rem;margin:0 0 20px;font-family:ui-monospace,Menlo,monospace}
section{background:var(--card);border:1px solid var(--edge);border-radius:12px;padding:14px 16px;margin-bottom:14px}
h2{font-size:.7rem;letter-spacing:.14em;text-transform:uppercase;color:var(--mute);margin:0 0 10px;font-family:ui-monospace,Menlo,monospace}
.row{display:flex;flex-wrap:wrap;gap:6px}
button{font:inherit;cursor:pointer;border:1px solid var(--edge);background:transparent;color:var(--mute);border-radius:999px;padding:6px 14px}
button:hover{border-color:var(--acc);color:var(--ink)}
button.on{background:var(--acc);border-color:var(--acc);color:var(--acc-ink)}
button.big{font-weight:600;color:var(--ink)}
.now{font-size:1.4rem;font-weight:600;margin:0 0 10px}
.now small{font-size:.8rem;font-weight:400;color:var(--mute);margin-left:8px}
label{display:grid;grid-template-columns:1fr auto;align-items:center;gap:10px;padding:6px 0;border-top:1px solid var(--edge);font-size:.92rem}
label:first-of-type{border-top:0}
label span{color:var(--mute)}
input,select{font:inherit;background:var(--bg);color:var(--ink);border:1px solid var(--edge);border-radius:6px;padding:5px 8px;width:120px}
input[type=checkbox]{width:auto;accent-color:var(--acc)}
.wx{display:flex;align-items:baseline;gap:14px;margin-bottom:10px}
.wx b{font-size:1.4rem}
.wx span{color:var(--mute)}
.foot{color:var(--mute);font-size:.8rem;font-family:ui-monospace,Menlo,monospace}
.msg{color:var(--acc);font-size:.85rem;min-height:1.2em;margin-top:8px}
</style></head><body><div class="wrap">
<h1>Desk Buddy</h1>
<p class="sub" id="sub">connecting&hellip;</p>

<section>
<h2>Right now</h2>
<p class="now"><span id="emotion">&hellip;</span><small id="state"></small></p>
<div class="row">
<button class="big" data-cmd="poke">Poke</button>
<button class="big" data-cmd="pet">Pet</button>
<button class="big" data-cmd="tap">Tap</button>
<button class="big" data-cmd="blink">Blink</button>
<button class="big" data-cmd="jolt">Startle</button>
<button class="big" id="sleepbtn" data-cmd="sleep">Sleep</button>
<button class="big" id="autobtn">Auto mood</button>
</div>
</section>

<section>
<h2>Expression</h2>
<div class="row" id="emos"></div>
<p class="msg">Picking one turns auto mood off; turn it back on to hand control back.</p>
</section>

<section>
<h2>Weather</h2>
<div class="wx"><b id="wxtemp">&ndash;</b><span id="wxkind">no reading yet</span></div>
<div class="row">
<button data-cmd="weather">Show now</button>
<button data-cmd="weather refresh">Refresh</button>
</div>
<p class="msg" id="wxmsg"></p>
</section>

<section>
<h2>Settings</h2>
<form id="settings">
<label>Bored after (s)<input name="bored" type="number" min="5" max="86400" step="1"></label>
<label>Sleep after (s)<input name="sleep" type="number" min="5" max="86400" step="1"></label>
<label>Brightness<input name="brightness" type="range" min="10" max="255"></label>
<label>Anti burn-in drift<input name="drift" type="checkbox"></label>
<label>Weather<input name="weather" type="checkbox"></label>
<label>Latitude<input name="lat" type="number" step="0.0001" min="-90" max="90"></label>
<label>Longitude<input name="lon" type="number" step="0.0001" min="-180" max="180"></label>
<label>Units<select name="units"><option value="F">&deg;F</option><option value="C">&deg;C</option></select></label>
<label>Fetch every (min)<input name="interval" type="number" min="5" max="1440"></label>
<label>Glance every (min)<span>0 = only when asked</span></label>
<label><span></span><input name="glance" type="number" min="0" max="1440"></label>
<div class="row" style="margin-top:10px"><button class="big on" type="submit">Save</button>
<button type="button" id="locate">Use my location</button></div>
<p class="msg" id="setmsg"></p>
</form>
</section>

<p class="foot" id="foot"></p>
</div>
<script>
const EMOS=["neutral","happy","excited","sad","angry","surprised","sleepy","love","curious","suspicious","dizzy","bored"];
const $=id=>document.getElementById(id);
let st=null, editing=false;

async function api(path, params){
  const r=await fetch(path,{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams(params||{})});
  return r.json();
}
async function cmd(c){ const r=await api("/api/cmd",{c}); refresh(); return r; }

const emos=$("emos");
EMOS.forEach(e=>{const b=document.createElement("button");b.textContent=e;b.dataset.emo=e;b.onclick=()=>cmd(e);emos.appendChild(b);});
document.querySelectorAll("[data-cmd]").forEach(b=>b.onclick=()=>cmd(b.dataset.cmd));
$("autobtn").onclick=()=>cmd(st&&st.auto?"auto off":"auto on");

function fill(s){
  st=s;
  $("sub").textContent=s.hostname+".local  ·  "+s.ip+"  ·  v"+s.version;
  $("emotion").textContent=s.emotion;
  $("state").textContent=(s.asleep?"asleep":s.petting?"being petted":s.auto?"auto mood":"manual")+"  ·  idle "+Math.round(s.idle)+"s";
  $("sleepbtn").textContent=s.asleep?"Wake":"Sleep"; $("sleepbtn").dataset.cmd=s.asleep?"wake":"sleep";
  $("autobtn").classList.toggle("on",s.auto);
  document.querySelectorAll("[data-emo]").forEach(b=>b.classList.toggle("on",b.dataset.emo===s.emotion));
  const w=s.weather;
  if(w.valid){$("wxtemp").textContent=w.temp+"°"+w.unit;$("wxkind").textContent=w.kind+(w.isDay?"":" (night)")+"  ·  "+Math.round(w.age/60)+" min ago";}
  else{$("wxtemp").textContent="–";$("wxkind").textContent=w.enabled?(w.busy?"fetching…":"no reading yet"):"off";}
  $("wxmsg").textContent=w.busy?"fetching…":(w.status&&w.status!==200?"last fetch failed ("+w.status+")":"");
  if(!editing){const f=$("settings"),c=s.settings;
    f.bored.value=c.bored;f.sleep.value=c.sleep;f.brightness.value=c.brightness;f.drift.checked=c.drift;
    f.weather.checked=c.weather;f.lat.value=c.lat;f.lon.value=c.lon;f.units.value=c.units;f.interval.value=c.interval;f.glance.value=c.glance;}
  $("foot").textContent="wifi "+s.rssi+" dBm  ·  up "+fmt(s.uptime)+"  ·  free "+Math.round(s.heap/1024)+" kB";
}
function fmt(s){const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return (d?d+"d ":"")+h+"h "+m+"m";}
async function refresh(){try{fill(await (await fetch("/api/status")).json());}catch(e){$("sub").textContent="no connection";}}

const form=$("settings");
form.addEventListener("focusin",()=>editing=true);
form.addEventListener("focusout",()=>setTimeout(()=>editing=false,1500));
form.onsubmit=async e=>{e.preventDefault();
  const f=form,r=await api("/api/settings",{bored:f.bored.value,sleep:f.sleep.value,brightness:f.brightness.value,
    drift:f.drift.checked?1:0,weather:f.weather.checked?1:0,lat:f.lat.value,lon:f.lon.value,units:f.units.value,
    interval:f.interval.value,glance:f.glance.value});
  $("setmsg").textContent=r.ok?"Saved":"Not saved: "+(r.error||"?");editing=false;refresh();
  setTimeout(()=>$("setmsg").textContent="",2500);};
$("locate").onclick=()=>{if(!navigator.geolocation){$("setmsg").textContent="No geolocation in this browser";return;}
  navigator.geolocation.getCurrentPosition(p=>{form.lat.value=p.coords.latitude.toFixed(4);form.lon.value=p.coords.longitude.toFixed(4);editing=true;$("setmsg").textContent="Filled in – press Save";},
  ()=>{$("setmsg").textContent="Location not available (needs https or localhost in most browsers)";});};

refresh(); setInterval(refresh,2000);
</script></body></html>)HTML";
