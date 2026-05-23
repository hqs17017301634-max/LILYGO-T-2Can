#pragma once
#ifdef ESP_PLATFORM
#include "platform/espidf_runtime.h"
#else
#include <Arduino.h>
#endif

static const char DASH_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>ev-open-can-tools</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
[data-theme="dark"]{
  --bg:#0d0d0d;--bg2:var(--bg);--card:#161616;--card2:#1e1e1e;
  --bd:#2a2a2a;--bd2:#333;
  --tx:#f0f0f0;--tx2:#999;--tx3:#555;
  --acc:#5b8fff;--accBg:rgba(91,143,255,.1);--accBd:rgba(91,143,255,.25);
  --ok:#3dba72;--okBg:rgba(61,186,114,.1);
  --err:#ff4f4f;--errBg:rgba(255,79,79,.08);--errBd:rgba(255,79,79,.2);
  --warn:#f5a623;
}
[data-theme="light"]{
  --bg:#f5f5f5;--bg2:var(--bg);--card:#fff;--card2:#f0f0f0;
  --bd:#e0e0e0;--bd2:#ccc;
  --tx:#111;--tx2:#555;--tx3:#999;
  --acc:#2563eb;--accBg:rgba(37,99,235,.08);--accBd:rgba(37,99,235,.2);
  --ok:#16a34a;--okBg:rgba(22,163,74,.08);
  --err:#dc2626;--errBg:rgba(220,38,38,.06);--errBd:rgba(220,38,38,.18);
  --warn:#d97706;
}
html{scroll-behavior:smooth}
body{background:var(--bg);color:var(--tx);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
  min-height:100vh;max-width:480px;margin:0 auto;font-size:14px;line-height:1.5;
  transition:background .2s,color .2s}
@media (min-width:700px){
  body{width:90vw;max-width:1180px}
}

/* Header */
.hdr{padding:20px 16px 0;display:flex;flex-direction:column;gap:4px}
.hdr-top{display:flex;align-items:center;justify-content:space-between}
.hdr-left{display:flex;align-items:center;gap:8px;flex-wrap:wrap;min-width:0}
.hdr-title{font-size:20px;font-weight:700;color:var(--tx)}
.hw-badge{padding:3px 8px;border-radius:5px;font-size:11px;font-weight:600;
  background:var(--accBg);border:1px solid var(--accBd);color:var(--acc)}
.gtw-badge{padding:3px 8px;border-radius:5px;font-size:11px;font-weight:600;
  background:var(--card);border:1px solid var(--bd2);color:var(--tx2)}
.gtw-badge.known{color:var(--ok);border-color:rgba(61,186,114,.25);background:var(--okBg)}
.theme-btn{padding:6px 10px;border:1px solid var(--bd2);border-radius:8px;
  background:var(--card);color:var(--tx2);font-size:12px;cursor:pointer;
  display:flex;align-items:center;gap:4px;transition:all .2s}
.theme-btn:hover{border-color:var(--acc);color:var(--acc)}
.hdr-status{display:flex;align-items:center;gap:6px;font-size:12px;color:var(--tx2)}
.sdot{width:7px;height:7px;border-radius:50%;flex-shrink:0;transition:all .4s}
.dot-on{background:var(--ok);box-shadow:0 0 8px var(--ok)}
.dot-off{background:var(--err)}
.dot-warn{background:var(--warn)}

/* FPS bar */
.fps-bar{margin:14px 16px 0;height:3px;background:var(--bd);border-radius:2px;overflow:hidden}
.fps-fill{height:100%;background:var(--acc);border-radius:2px;transition:width .5s;width:0%}

/* Status grid */
.stat-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin:14px 16px 0}
.stat{background:var(--card);border:1px solid var(--bd);border-radius:10px;padding:10px 12px}
.stat-lbl{font-size:10px;color:var(--tx3);text-transform:uppercase;letter-spacing:.8px;margin-bottom:3px}
.stat-val{font-size:14px;font-weight:600;color:var(--tx)}
.v-ok{color:var(--ok)}.v-err{color:var(--err)}.v-acc{color:var(--acc)}.v-dim{color:var(--tx3)}.v-warn{color:var(--warn)}
.stat-wide{grid-column:span 3}
.sys-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.sys-item{background:var(--bg2);border:1px solid var(--bd);border-radius:8px;padding:8px 10px;min-width:0}
.sys-lbl{font-size:10px;color:var(--tx3);text-transform:uppercase;letter-spacing:.5px;margin-bottom:2px}
.sys-val{font-size:12px;font-weight:600;color:var(--tx);word-break:break-word}
.sys-wide{grid-column:span 2}
.sys-bar{height:4px;background:var(--bd);border-radius:2px;overflow:hidden;margin-top:6px}
.sys-fill{height:100%;background:var(--acc);border-radius:2px;transition:width .3s;width:0}
.task-table{width:100%;border-collapse:collapse;margin-top:4px;font-family:'SF Mono','Courier New',monospace;font-size:11px;table-layout:fixed}
.task-table th{color:var(--tx3);font-size:9px;text-transform:uppercase;letter-spacing:.5px;text-align:left;font-weight:600;padding:3px 4px;border-bottom:1px solid var(--bd)}
.task-table td{padding:4px;border-bottom:1px solid var(--bd);color:var(--tx2);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.task-table tr:last-child td{border-bottom:0}
.task-table .task-name{color:var(--tx);font-weight:600;width:34%}
.task-table .task-core{width:15%}.task-table .task-cpu{width:18%}.task-table .task-stack{width:18%}.task-table .task-state{width:15%}
.sys-monitor{display:flex;align-items:center;justify-content:flex-end;gap:8px}
.sys-monitor span{white-space:nowrap}
.sys-monitor .tgl{margin-left:0}
/* Divider */
hr{border:none;border-top:1px solid var(--bd);margin:16px}

/* Cards */
.card{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:16px;margin:0 16px 12px;overflow:hidden}
.card-hdr{display:grid;grid-template-columns:minmax(0,1fr) auto auto;align-items:center;column-gap:8px;margin-bottom:14px}
.card-title{font-size:13px;font-weight:600;color:var(--tx);text-transform:uppercase;letter-spacing:.5px;min-width:0}
.card-meta{font-size:11px;color:var(--tx3);justify-self:end;text-align:right;min-width:0}
.card-min-btn{padding:4px 8px;font-size:10px;justify-self:end}
.card.collapsed{padding-bottom:12px}
.card.collapsed .card-hdr{margin-bottom:0}
.card.collapsed>:not(.card-hdr){display:none !important}
.subsec{margin-top:14px;padding-top:12px;border-top:1px solid var(--bd)}
.subsec:first-child{margin-top:0;padding-top:0;border-top:none}
.subsec-head{display:grid;grid-template-columns:minmax(110px,1fr) auto auto;align-items:center;column-gap:8px;margin-bottom:8px}
.subsec-title{font-size:13px;font-weight:600;color:var(--tx);min-width:0;word-break:keep-all}
.title-help{display:inline-flex;align-items:center;justify-content:center;width:16px;height:16px;margin-left:6px;border:1px solid var(--bd2);border-radius:50%;font-size:10px;font-weight:700;color:var(--tx3);cursor:pointer;vertical-align:middle;line-height:1;background:transparent}
.title-help:hover{border-color:var(--accBd);color:var(--acc);background:var(--accBg)}
.info-box{margin-bottom:10px;padding:10px 12px;background:var(--bg2);border:1px solid var(--bd);border-radius:9px;font-size:12px;color:var(--tx3);line-height:1.6}
.info-box a{color:var(--acc);text-decoration:none}
.inline-help-panel{display:none;margin:8px 0 0;padding:10px 12px;background:var(--bg2);border:1px solid var(--bd);border-radius:9px;font-size:12px;color:var(--tx3);line-height:1.6}
.inline-help-panel.show{display:block}
.subsec-meta{font-size:11px;color:var(--tx3);justify-self:end;text-align:right;min-width:0}
.subsec-btn{padding:4px 8px;font-size:10px;justify-self:end}
.subsec.collapsed .subsec-head{margin-bottom:0}
.subsec.collapsed .subsec-body{display:none}

/* HW seg */
.hw-seg{display:flex;background:var(--card2);border:1px solid var(--bd);border-radius:9px;padding:3px;gap:2px}
.hw-btn{flex:1;padding:8px;border:none;border-radius:7px;font-size:12px;font-weight:600;
  cursor:pointer;background:transparent;color:var(--tx2);transition:all .18s;font-family:inherit}
.hw-btn.active{background:var(--card);color:var(--acc);border:1px solid var(--accBd);
  box-shadow:0 1px 4px rgba(0,0,0,.15)}
.hw-btn:hover:not(.active){background:var(--bd);color:var(--tx)}
.profile-wrap{margin-top:12px}
.profile-label{font-size:11px;color:var(--tx3);margin-bottom:6px}
.profile-group.hidden{display:none}
.profile-note{font-size:10px;color:var(--tx3);margin-top:6px}

/* Speed pills */
.pills{display:flex;gap:6px;flex-wrap:wrap}
/* Settings rows */
.setting-row{display:flex;align-items:center;justify-content:space-between;
  padding:12px 0;border-bottom:1px solid var(--bd)}
.setting-row:last-of-type{border-bottom:none;padding-bottom:0}
.setting-row:first-of-type{padding-top:0}
.setting-info{flex:1;min-width:0}
.setting-name{font-size:13px;font-weight:500;color:var(--tx)}
.setting-desc{font-size:11px;color:var(--tx3);margin-top:2px}
.hw4-only.hidden{display:none}

/* Toggle */
.tgl{position:relative;width:44px;height:24px;flex-shrink:0;margin-left:12px}
.tgl input{opacity:0;width:0;height:0;position:absolute}
.tgl-track{position:absolute;inset:0;background:var(--bd2);border-radius:24px;cursor:pointer;transition:all .22s}
.tgl-thumb{position:absolute;top:3px;left:3px;width:18px;height:18px;background:#fff;
  border-radius:50%;transition:all .22s;box-shadow:0 1px 3px rgba(0,0,0,.3)}
.tgl input:checked~.tgl-track{background:var(--acc)}
.tgl input:checked~.tgl-track .tgl-thumb{transform:translateX(20px)}
.tgl input:disabled~.tgl-track{opacity:.35;cursor:not-allowed}

/* Sniffer */
.sniff-ctrl{display:flex;gap:6px;margin-bottom:8px}
.sniff-input{flex:1;background:var(--bg);border:1px solid var(--bd);border-radius:8px;
  padding:7px 10px;color:var(--tx);font-size:12px;font-family:inherit;transition:border .2s}
.sniff-input{width:100%;min-width:0;box-sizing:border-box;} 
.sniff-input:focus{outline:none;border-color:var(--acc)}
.sniff-input::placeholder{color:var(--tx3)}
.sniff-btn{padding:7px 12px;background:transparent;border:1px solid var(--bd);border-radius:8px;
  color:var(--tx2);font-size:11px;font-weight:600;cursor:pointer;transition:all .18s;font-family:inherit}
.sniff-btn.paused{border-color:var(--warn);color:var(--warn)}
.sniff-btn:hover:not(.paused){border-color:var(--bd2);color:var(--tx)}
.gateway-profile-btn.active,.gateway-upstream-btn.active{background:var(--accBg);border-color:var(--acc);color:var(--acc);box-shadow:0 0 0 1px var(--accBd) inset}
.sniff-box{background:var(--bg);border:1px solid var(--bd);border-radius:9px;
  max-height:250px;overflow-y:auto;font-family:'SF Mono','Courier New',monospace}
.sniff-box::-webkit-scrollbar{width:4px}
.sniff-box::-webkit-scrollbar-thumb{background:var(--bd2);border-radius:4px}
.sniff-row{display:grid;grid-template-columns:38px 72px 1fr;gap:8px;
  padding:6px 10px;border-bottom:1px solid var(--bd);font-size:11px;align-items:start}
.sniff-row:last-child{border-bottom:none}
.sniff-row.hi{border-left:2px solid var(--acc);padding-left:8px}
.s-ts{color:var(--tx3);font-size:10px;padding-top:1px}
.s-id{color:var(--acc);font-weight:700}
.s-data{color:var(--tx2);word-break:break-all}
.s-name{color:var(--ok);font-size:10px;margin-top:2px}

/* EFLG */
.eflg-row{display:flex;flex-wrap:wrap;gap:5px;margin-top:10px}
.eflg-pill{padding:3px 8px;border-radius:5px;font-size:10px;font-weight:600;letter-spacing:.3px}
.eflg-ok{background:var(--okBg);color:var(--ok)}
.eflg-warn{background:rgba(245,166,35,.1);color:var(--warn)}
.eflg-err{background:var(--errBg);color:var(--err)}

/* Mux table */
.mux-tbl{width:100%;border-collapse:collapse;font-size:12px;margin-top:10px}
.mux-tbl th{color:var(--tx3);font-size:10px;text-transform:uppercase;letter-spacing:.8px;
  text-align:left;padding:4px 8px;border-bottom:1px solid var(--bd);font-weight:500}
.mux-tbl td{padding:5px 8px;color:var(--tx2);border-bottom:1px solid var(--bd)}
.mux-tbl tr:last-child td{border-bottom:none}
.mux-tbl td:first-child{color:var(--acc);font-weight:600}

/* Last write check */
.probe-status{font-size:13px;font-weight:600}
.probe-note{font-size:11px;color:var(--tx3);line-height:1.6;margin-top:10px}
.probe-block{margin-top:12px;padding-top:12px;border-top:1px solid var(--bd)}
.probe-meta{font-size:11px;color:var(--tx3);margin-bottom:4px}
.probe-label{font-size:10px;color:var(--tx3);text-transform:uppercase;letter-spacing:.8px;margin-bottom:6px}
.probe-hex{font-family:'SF Mono','Courier New',monospace;font-size:12px;color:var(--tx2);word-break:break-all}

/* Buttons */
.btn-row{display:flex;gap:8px;margin-top:14px}
.btn{flex:1;padding:10px;border:1px solid;border-radius:9px;background:transparent;
  font-family:inherit;font-size:12px;font-weight:600;cursor:pointer;transition:all .18s;letter-spacing:.3px}
.btn-stop{border-color:var(--errBd);color:var(--err)}
.btn-stop:hover{background:var(--errBg)}
.btn-reboot{border-color:var(--bd2);color:var(--tx2)}
.btn-reboot:hover{border-color:var(--acc);color:var(--acc)}
.stat-grid>.btn{min-height:auto;padding:10px 12px;border-radius:10px;background:var(--card);text-align:left;
  display:flex;align-items:flex-start;justify-content:flex-start;font-size:14px;font-weight:600;letter-spacing:0;line-height:1.35}
.stat-grid>.btn:hover{background:var(--card2)}

/* Confirm modal */
.modal-backdrop{position:fixed;inset:0;display:none;align-items:center;justify-content:center;
  padding:16px;background:rgba(0,0,0,.55);z-index:9999}
.modal-card{width:min(100%,360px);background:var(--card);border:1px solid var(--bd2);
  border-radius:12px;padding:16px;box-shadow:0 16px 40px rgba(0,0,0,.35)}
.modal-title{font-size:14px;font-weight:700;color:var(--tx)}
.modal-msg{margin-top:8px;font-size:12px;color:var(--tx2);line-height:1.6;white-space:pre-wrap}
.modal-actions{display:flex;justify-content:flex-end;gap:8px;margin-top:14px}
.modal-btn-primary{background:var(--accBg);border-color:var(--accBd);color:var(--acc)}
.modal-btn-primary:hover{background:var(--acc);color:#fff}
.owner-modal-card{width:min(100%,390px);border-color:var(--accBd);box-shadow:0 18px 48px rgba(0,0,0,.38)}
.owner-modal-title{font-size:17px;font-weight:800;color:var(--acc);letter-spacing:.3px}
.owner-modal-subtitle{margin-top:6px;font-size:14px;font-weight:700;color:var(--tx)}
.owner-modal-msg{margin-top:12px;font-size:13px;color:var(--tx2);line-height:1.8;white-space:pre-line}
.owner-modal-msg .ok{color:var(--ok);font-weight:700}
.dns-modal-card{width:min(100%,640px)}
.dns-modal-list{margin-top:10px;max-height:60vh;overflow:auto;border:1px solid var(--bd);border-radius:9px;padding:8px;background:var(--bg)}
.dns-row{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:10px;align-items:center;padding:10px 8px;border-bottom:1px solid var(--bd)}
.dns-row:last-child{border-bottom:none}
.dns-domain{min-width:0;overflow:hidden;text-overflow:ellipsis;color:var(--tx);font-family:'SF Mono','Courier New',monospace;font-size:12px}
.dns-count{color:var(--tx3);font-size:10px;margin-left:6px}
.dns-state{font-size:11px;font-weight:600;white-space:nowrap}
.dns-state.err{color:var(--err)}
.dns-state.ok{color:var(--ok)}
.dns-state.dim{color:var(--tx3)}

/* OTA upload */
.ota-drop{border:2px dashed var(--bd2);border-radius:10px;padding:24px 16px;
  text-align:center;cursor:pointer;transition:all .2s;background:var(--bg)}
.ota-drop:hover,.ota-drop.drag{border-color:var(--acc);background:var(--accBg)}
.ota-drop input{display:none}
.ota-icon{font-size:24px;margin-bottom:8px}
.ota-text{font-size:13px;font-weight:500;color:var(--tx2);margin-bottom:3px}
.ota-sub{font-size:11px;color:var(--tx3)}
.ota-progress{margin-top:12px;display:none}
.ota-bar{height:4px;background:var(--bd);border-radius:2px;overflow:hidden;margin-bottom:6px}
.ota-fill{height:100%;background:var(--acc);border-radius:2px;transition:width .3s;width:0%}
.ota-status{font-size:11px;color:var(--acc);text-align:center}
.ota-btn{width:100%;margin-top:10px;padding:10px;border:1px solid var(--accBd);border-radius:9px;
  background:var(--accBg);color:var(--acc);font-family:inherit;font-size:13px;font-weight:600;
  cursor:pointer;transition:all .2s;display:none}
.ota-btn:hover{background:var(--acc);color:#fff}

/* Log */
.log-box{background:var(--bg);border:1px solid var(--bd);border-radius:9px;padding:10px 12px;
  font-family:'SF Mono','Courier New',monospace;font-size:11px;color:var(--tx2);
  max-height:180px;overflow-y:auto;line-height:1.9;white-space:pre-wrap;word-break:break-all}
.log-box::-webkit-scrollbar{width:4px}
.log-box::-webkit-scrollbar-thumb{background:var(--bd2);border-radius:4px}
.lf{color:var(--ok)}.lh{color:var(--acc)}.le{color:var(--err)}.lc{color:var(--warn)}.lo{color:var(--tx2)}

/* Recorder */
.rec-bar{height:4px;background:var(--bd);border-radius:2px;overflow:hidden;margin-bottom:6px}
.rec-fill{height:100%;background:var(--err);border-radius:2px;transition:width .3s;width:0%}
.rec-info{display:flex;justify-content:space-between;font-size:11px;color:var(--tx3);margin-bottom:10px}

/* Warning */
.warn-bar{margin:0 16px 14px;padding:10px 14px;border-radius:9px;
  background:var(--errBg);border:1px solid var(--errBd);font-size:11px;color:var(--err);line-height:1.7}
.foot{text-align:center;padding:8px 16px 20px;font-size:11px;color:var(--tx3)}
body:not(.can-debug-on) .can-debug-panel{display:none !important}
</style>
</head>
<body>

<div class="hdr">
  <div class="hdr-top">
    <div class="hdr-left">
      <div class="hdr-title">ev-open-can-tools</div>
      <span class="hw-badge" id="hw-badge">HW3</span>
      <span class="gtw-badge" id="gtw-badge" title="GTW_autopilot">GTW &mdash;</span>
    </div>
    <button class="theme-btn" onclick="toggleLanguage()" id="lang-btn">中文</button>
    <button class="theme-btn" onclick="toggleTheme()" id="theme-btn">&#9788; Light</button>
  </div>
  <div class="hdr-status">
    <span class="sdot dot-off" id="dot"></span>
    <span id="hdr-desc">Waiting for CAN frames</span>
  </div>
</div>

<div class="fps-bar"><div class="fps-fill" id="fps-fill"></div></div>

<div class="stat-grid">
  <div class="stat"><div class="stat-lbl">CAN Bus</div><div class="stat-val" id="s-can">Offline</div></div>
  <div class="stat"><div class="stat-lbl">FSD Switch</div><div class="stat-val v-dim" id="s-inj">--</div></div>
  <div class="stat"><div class="stat-lbl" title="Frames received per second">CAN Frame Rate</div><div class="stat-val v-dim" id="s-fps">0.0 Hz</div></div>
  <div class="stat"><div class="stat-lbl">RX</div><div class="stat-val v-acc" id="s-rx">0</div></div>
  <div class="stat"><div class="stat-lbl">TX</div><div class="stat-val v-acc" id="s-tx">0</div></div>
  <div class="stat"><div class="stat-lbl">TX Errors</div><div class="stat-val v-dim" id="s-txerr">0</div></div>
  <div class="stat"><div class="stat-lbl">Follow dist</div><div class="stat-val v-dim" id="s-fd">--</div></div>
  <div class="stat"><div class="stat-lbl">Profile</div><div class="stat-val v-dim" id="s-prof">--</div></div>
  <div class="stat"><div class="stat-lbl">Limit Offset</div><div class="stat-val v-dim" id="s-soff">0</div></div>
  <div class="stat"><div class="stat-lbl">Uptime</div><div class="stat-val v-dim" id="s-up">0s</div></div>
  <button class="btn" id="btn-fsd-toggle" onclick="toggleFsdTopButton()">Turn FSD On</button>
  <button class="btn btn-reboot" onclick="reboot()">Reboot</button>
</div>

<div style="height:12px"></div>

<div class="card">
  <div class="card-hdr">
    <div class="card-title">System Status <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Hardware and runtime health reported by the ESP32 firmware.">i</span></div>
    <div class="card-meta sys-monitor"><span id="sys-summary">Monitoring off</span><label class="tgl" title="Enable live hardware status sampling"><input type="checkbox" id="sys-monitor-tgl" onchange="toggleSystemMonitor()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label></div>
  </div>
  <div class="sys-grid">
    <div class="sys-item"><div class="sys-lbl">Chip</div><div class="sys-val" id="sys-chip">--</div></div>
    <div class="sys-item"><div class="sys-lbl">CPU</div><div class="sys-val" id="sys-cpu">--</div></div>
    <div class="sys-item sys-wide">
      <div class="sys-lbl">CPU Load</div><div class="sys-val" id="sys-cpu-load">--</div>
      <div class="sys-bar"><div class="sys-fill" id="sys-cpu0-fill"></div></div>
      <div class="sys-bar" style="margin-top:4px"><div class="sys-fill" id="sys-cpu1-fill"></div></div>
    </div>
    <div class="sys-item sys-wide">
      <div class="sys-lbl">Task Load</div>
      <table class="task-table">
        <thead><tr><th class="task-name">task</th><th class="task-core">core</th><th class="task-cpu">cpu%</th><th class="task-stack">stack</th><th class="task-state">state</th></tr></thead>
        <tbody id="sys-task-rows"><tr><td colspan="5" class="v-dim">--</td></tr></tbody>
      </table>
    </div>
    <div class="sys-item sys-wide"><div class="sys-lbl">Board Specs</div><div class="sys-val" id="sys-board">--</div></div>
    <div class="sys-item"><div class="sys-lbl">Temperature</div><div class="sys-val" id="sys-temp">--</div></div>
    <div class="sys-item"><div class="sys-lbl">Reset</div><div class="sys-val" id="sys-reset">--</div></div>
    <div class="sys-item sys-wide">
      <div class="sys-lbl">Heap RAM</div><div class="sys-val" id="sys-heap">--</div>
      <div class="sys-bar"><div class="sys-fill" id="sys-heap-fill"></div></div>
    </div>
    <div class="sys-item"><div class="sys-lbl">Largest Block</div><div class="sys-val" id="sys-largest">--</div></div>
    <div class="sys-item"><div class="sys-lbl">Min Free Heap</div><div class="sys-val" id="sys-minheap">--</div></div>
    <div class="sys-item"><div class="sys-lbl">PSRAM</div><div class="sys-val" id="sys-psram">--</div></div>
    <div class="sys-item"><div class="sys-lbl">Tasks</div><div class="sys-val" id="sys-tasks">--</div></div>
    <div class="sys-item sys-wide">
      <div class="sys-lbl">Flash</div><div class="sys-val" id="sys-flash">--</div>
      <div class="sys-bar"><div class="sys-fill" id="sys-app-fill"></div></div>
    </div>
    <div class="sys-item sys-wide">
      <div class="sys-lbl">SPIFFS</div><div class="sys-val" id="sys-spiffs">--</div>
      <div class="sys-bar"><div class="sys-fill" id="sys-spiffs-fill"></div></div>
    </div>
    <div class="sys-item"><div class="sys-lbl">WiFi RSSI</div><div class="sys-val" id="sys-rssi">--</div></div>
    <div class="sys-item"><div class="sys-lbl">WiFi Mode</div><div class="sys-val" id="sys-wifi-mode">--</div></div>
    <div class="sys-item"><div class="sys-lbl">AP Clients</div><div class="sys-val" id="sys-apclients">--</div></div>
    <div class="sys-item"><div class="sys-lbl">Bluetooth LE</div><div class="sys-val" id="sys-ble">--</div></div>
    <div class="sys-item sys-wide"><div class="sys-lbl">Wireless</div><div class="sys-val" id="sys-wireless">--</div></div>
    <div class="sys-item sys-wide"><div class="sys-lbl">MAC / Firmware</div><div class="sys-val" id="sys-fw">--</div></div>
  </div>
</div>

<div style="height:12px"></div>

<div class="card">
  <div class="card-hdr">
    <div class="card-title">Bus2 Sniffer (X197 9/10) <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Standard CAN IDs seen on the secondary bus (MCP2515). Use this to confirm the 9/10 bus and find target frames like 0x249 (stalk).">i</span></div>
    <div class="card-meta"><span id="bus2-count">0</span> IDs</div>
  </div>
  <table class="task-table">
    <thead><tr><th class="task-name">ID</th><th class="task-core">DLC</th><th class="task-cpu">count</th><th class="task-state">last data</th></tr></thead>
    <tbody id="bus2-rows"><tr><td colspan="4" class="v-dim">waiting&hellip;</td></tr></tbody>
  </table>
  <div class="sniff-ctrl" style="margin-top:8px">
    <button class="sniff-btn" onclick="startRec('249,3E9,3F5')" title="Start a filtered recording that captures only the Tesla lighting/stalk frames (0x249 stalk, 0x3E9 DAS body controls, 0x3F5 lighting status) on both buses. Operate the stalk / drive in FSD, then Stop and Download CSV in the CAN section ▸ Recorder for offline checksum and counter analysis.">Capture lighting IDs (249/3E9/3F5)</button>
  </div>
</div>

<div class="card">
  <div class="card-hdr">
    <div class="card-title">Configuration <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Device settings for hardware mode, WiFi, CAN pins, logging and backup.">i</span></div>
    <div class="card-meta">Device settings</div>
  </div>

  <div class="subsec" data-subkey="config-hardware">
    <div class="subsec-head">
      <div class="subsec-title">Hardware <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Select the autopilot hardware generation and matching speed profile set.">i</span></div>
      <div class="subsec-meta">Autopilot generation</div>
    </div>
    <div class="subsec-body">
      <div class="hw-seg" id="hw-seg">
        <button class="hw-btn" data-v="0" onclick="setHW(0)">Legacy</button>
        <button class="hw-btn active" data-v="1" onclick="setHW(1)">HW3</button>
        <button class="hw-btn" data-v="2" onclick="setHW(2)">HW4</button>
      </div>
      <div class="profile-wrap">
        <div class="profile-label">Profile</div>
        <div class="profile-group" id="sp3-group">
          <div class="hw-seg" id="sp3-seg">
            <button class="hw-btn" data-v="-1" onclick="setProfileAuto()">Auto</button>
            <button class="hw-btn" data-v="0" onclick="setProfile(0)">Chill</button>
            <button class="hw-btn" data-v="1" onclick="setProfile(1)">Normal</button>
            <button class="hw-btn" data-v="2" onclick="setProfile(2)">Hurry</button>
          </div>
        </div>
        <div class="profile-group hidden" id="sp4-group">
          <div class="hw-seg" id="sp4-seg">
            <button class="hw-btn" data-v="-1" onclick="setProfileAuto()">Auto</button>
            <button class="hw-btn" data-v="0" onclick="setProfile(0)">Chill</button>
            <button class="hw-btn" data-v="1" onclick="setProfile(1)">Normal</button>
            <button class="hw-btn" data-v="2" onclick="setProfile(2)">Hurry</button>
            <button class="hw-btn" data-v="3" onclick="setProfile(3)">Max</button>
            <button class="hw-btn" data-v="4" onclick="setProfile(4)">Sloth</button>
          </div>
        </div>
        <div class="profile-note" id="profile-note">Available profiles depend on the selected hardware.</div>
      </div>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-name">AP/EAP Auto Restore</div>
          <div class="setting-desc">Optional 0x293 Autosteer enable restore after AP/EAP ACC drop. Default off.</div>
        </div>
        <label class="tgl"><input type="checkbox" id="ap-restore-tgl" onchange="saveApRestore()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
    </div>
  </div>


  <div class="subsec" id="hw3-speed-section" data-subkey="config-hw3-speed">
    <div class="subsec-head">
      <div class="subsec-title">HW3 Custom Speed Limit <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Writes only the 1021 mux2 offset when custom speed is enabled. Maximum target is limited to +50% of the detected speed limit so FSD activation mux0 remains untouched.">i</span></div>
      <div class="subsec-meta" id="hw3-speed-meta">Off</div>
    </div>
    <div class="subsec-body">
      <div class="setting-row" style="padding-top:0">
        <div class="setting-info">
          <div class="setting-name">Custom table</div>
          <div class="setting-desc">30/40/50/60/70 km/h buckets. Max +50% target: 45/60/75/90/105 km/h.</div>
        </div>
        <label class="tgl"><input type="checkbox" id="hw3-cust-tgl" onchange="saveHw3Speed()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-top:8px">
        <div class="stat" style="padding:6px"><div class="stat-lbl">30-></div><input class="sniff-input" id="hw3-ct-0" type="number" min="0" max="45" value="45" onchange="saveHw3Speed()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">40-></div><input class="sniff-input" id="hw3-ct-1" type="number" min="0" max="60" value="60" onchange="saveHw3Speed()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">50-></div><input class="sniff-input" id="hw3-ct-2" type="number" min="0" max="75" value="75" onchange="saveHw3Speed()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">60-></div><input class="sniff-input" id="hw3-ct-3" type="number" min="0" max="90" value="90" onchange="saveHw3Speed()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">70-></div><input class="sniff-input" id="hw3-ct-4" type="number" min="0" max="105" value="105" onchange="saveHw3Speed()" style="width:100%;text-align:right"></div>
      </div>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-name">High-speed boost (&gt;=80 km/h)</div>
          <div class="setting-desc">80/100/120 km/h buckets. Max +50% target: 120/150/180 km/h.</div>
        </div>
        <label class="tgl"><input type="checkbox" id="hw3-hs-tgl" onchange="saveHw3Speed()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div style="display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:8px">
        <div class="stat" style="padding:6px"><div class="stat-lbl">80-></div><input class="sniff-input" id="hw3-hs-0" type="number" min="0" max="120" value="90" onchange="saveHw3Speed()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">100-></div><input class="sniff-input" id="hw3-hs-1" type="number" min="0" max="150" value="110" onchange="saveHw3Speed()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">120-></div><input class="sniff-input" id="hw3-hs-2" type="number" min="0" max="180" value="130" onchange="saveHw3Speed()" style="width:100%;text-align:right"></div>
      </div>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-name">Wire encoding</div>
          <div class="setting-desc">PCT4=current, KPH5=legacy fleets</div>
        </div>
        <select class="sniff-input" id="hw3-enc" onchange="saveHw3Speed()" style="width:96px;flex:0 0 auto">
          <option value="1">PCT4</option>
          <option value="0">KPH5</option>
        </select>
      </div>
      <div style="display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:10px">
        <div class="stat" style="padding:8px"><div class="stat-lbl">Fused</div><div class="stat-val" id="hw3-fused">0</div></div>
        <div class="stat" style="padding:8px"><div class="stat-lbl">Stock limit offset</div><div class="stat-val" id="hw3-stock-off">0</div></div>
        <div class="stat" style="padding:8px"><div class="stat-lbl">Write raw</div><div class="stat-val" id="hw3-tgt-raw">0</div></div>
      </div>
      <div style="font-size:11px;color:var(--tx3);margin-top:6px" id="hw3-speed-status"></div>
    </div>
  </div>

  <div class="subsec" id="legacy-mpp-section" data-subkey="config-legacy-mpp">
    <div class="subsec-head">
      <div class="subsec-title">Legacy Custom Speed Limit <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Raise UI_mppSpeedLimit on CAN 760 byte 6 to a target km/h based on what the gateway is currently sending. Same bucket layout as HW3. Only writes when target is higher than current - never lowers.">i</span></div>
      <div class="subsec-meta" id="legacy-mpp-meta">Off</div>
    </div>
    <div class="subsec-body">
      <div class="setting-row" style="padding-top:0">
        <div class="setting-info">
          <div class="setting-name">Master enable</div>
          <div class="setting-desc">Allow this module to write to UI_mppSpeedLimit</div>
        </div>
        <label class="tgl"><input type="checkbox" id="legacy-mpp-tgl" onchange="saveLegacyMpp()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-name">Custom table</div>
          <div class="setting-desc">30/40/50/60/70 km/h buckets. Max +50% target: 45/60/75/90/105 km/h.</div>
        </div>
        <label class="tgl"><input type="checkbox" id="legacy-mpp-cust-tgl" onchange="saveLegacyMpp()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:6px;margin-top:8px">
        <div class="stat" style="padding:6px"><div class="stat-lbl">30-></div><input class="sniff-input" id="legacy-mpp-ct-0" type="number" min="0" max="45" value="45" onchange="saveLegacyMpp()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">40-></div><input class="sniff-input" id="legacy-mpp-ct-1" type="number" min="0" max="60" value="60" onchange="saveLegacyMpp()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">50-></div><input class="sniff-input" id="legacy-mpp-ct-2" type="number" min="0" max="75" value="75" onchange="saveLegacyMpp()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">60-></div><input class="sniff-input" id="legacy-mpp-ct-3" type="number" min="0" max="90" value="90" onchange="saveLegacyMpp()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">70-></div><input class="sniff-input" id="legacy-mpp-ct-4" type="number" min="0" max="105" value="105" onchange="saveLegacyMpp()" style="width:100%;text-align:right"></div>
      </div>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-name">High-speed boost (&gt;=80 km/h)</div>
          <div class="setting-desc">80/100/120 km/h buckets. Max target: 120/150/155 km/h.</div>
        </div>
        <label class="tgl"><input type="checkbox" id="legacy-mpp-hs-tgl" onchange="saveLegacyMpp()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div style="display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:8px">
        <div class="stat" style="padding:6px"><div class="stat-lbl">80-></div><input class="sniff-input" id="legacy-mpp-hs-0" type="number" min="0" max="120" value="90" onchange="saveLegacyMpp()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">100-></div><input class="sniff-input" id="legacy-mpp-hs-1" type="number" min="0" max="150" value="110" onchange="saveLegacyMpp()" style="width:100%;text-align:right"></div>
        <div class="stat" style="padding:6px"><div class="stat-lbl">120-></div><input class="sniff-input" id="legacy-mpp-hs-2" type="number" min="0" max="155" value="130" onchange="saveLegacyMpp()" style="width:100%;text-align:right"></div>
      </div>
      <div style="display:grid;grid-template-columns:repeat(2,1fr);gap:6px;margin-top:10px">
        <div class="stat" style="padding:8px"><div class="stat-lbl">Bus raw</div><div class="stat-val" id="legacy-mpp-bus-raw">0</div></div>
        <div class="stat" style="padding:8px"><div class="stat-lbl">Sent raw</div><div class="stat-val" id="legacy-mpp-sent-raw">0</div></div>
      </div>
      <div style="font-size:11px;color:var(--tx3);margin-top:6px" id="legacy-mpp-status"></div>
    </div>
  </div>

  <div class="subsec" id="hw3-slew-section" data-subkey="config-hw3-slew">
    <div class="subsec-head">
      <div class="subsec-title">HW3 Offset Slew <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Limits downward HW3 mux 2 offset changes sent by the built-in FSD chain.">i</span></div>
      <div class="subsec-meta" id="hw3-slew-meta">Off</div>
    </div>
    <div class="subsec-body">
      <div class="setting-row" style="padding-top:0">
        <div class="setting-info">
          <div class="setting-name">Ramp-down limiter</div>
          <div class="setting-desc">Opt-in only; increases still pass immediately</div>
        </div>
        <label class="tgl"><input type="checkbox" id="hw3-slew-tgl" onchange="saveHw3Slew()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-name">Offset drop rate</div>
          <div class="setting-desc" id="hw3-slew-rate-hint">Default 25%/s, max 25%/s</div>
        </div>
        <input class="sniff-input" id="hw3-slew-rate" type="number" min="1" max="25" value="25" onchange="saveHw3Slew()" style="width:72px;text-align:right;flex:0 0 auto">
      </div>
      <div style="display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:10px">
        <div class="stat" style="padding:8px"><div class="stat-lbl">Target</div><div class="stat-val" id="hw3-slew-target">0</div></div>
        <div class="stat" style="padding:8px"><div class="stat-lbl">Last</div><div class="stat-val" id="hw3-slew-last">0</div></div>
        <div class="stat" style="padding:8px"><div class="stat-lbl">Capped</div><div class="stat-val" id="hw3-slew-count">0</div></div>
      </div>
      <div style="font-size:11px;color:var(--tx3);margin-top:6px" id="hw3-slew-status"></div>
    </div>
  </div>

  <div class="subsec" data-subkey="config-wifi-hotspot">
    <div class="subsec-head">
      <div class="subsec-title">WiFi Hotspot <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" data-help-target="ap-info" title="Configure the device hotspot name, password and visibility. Saved in NVS.">i</span></div>
      <div class="subsec-meta"><span id="ap-stored" style="margin-right:8px"></span><span id="ap-clients">0 clients</span></div>
    </div>
    <div class="subsec-body">
      <div id="ap-info" class="info-box" style="display:none">
        Stored in NVS (non-volatile storage). The SSID and password survive firmware updates and reboots. Only a full factory erase via USB clears them.
      </div>
      <div class="setting-desc" style="margin-bottom:8px">Change the WiFi hotspot name and password</div>
      <div style="display:flex;gap:6px;margin-bottom:6px">
        <input class="sniff-input" id="ap-ssid" placeholder="Hotspot Name" style="flex:1">
        <input class="sniff-input" id="ap-pass" placeholder="New Password (min 8)" type="password" style="flex:1">
      </div>
      <div class="setting-row" style="padding:8px 0">
        <div class="setting-info">
          <div class="setting-name">Hide SSID</div>
          <div class="setting-desc">Don't broadcast the hotspot name &mdash; clients must enter it manually</div>
        </div>
        <label class="tgl"><input type="checkbox" id="ap-hidden"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div style="display:flex;gap:6px;align-items:center">
        <button class="sniff-btn" onclick="saveAP()">Save</button>
        <span style="font-size:11px;color:var(--tx3)" id="ap-status"></span>
      </div>
      <div style="font-size:10px;color:var(--tx3);margin-top:6px">Changes take effect after reboot. Leave password empty to keep current.</div>
    </div>
  </div>

  <div class="subsec" data-subkey="config-wifi-internet">
    <div class="subsec-head">
      <div class="subsec-title">WiFi Internet <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Up to 4 saved networks. The device tries each in turn until one connects.">i</span></div>
      <div class="subsec-meta"><span id="wifi-status">Not configured</span></div>
    </div>
    <div class="subsec-body">
      <div class="setting-desc" style="margin-bottom:8px">Save up to 4 networks (e.g. home + phone hotspot). Device tries each in turn. Stored in NVS &mdash; survives firmware updates.</div>
      <div id="wifi-saved-list" style="margin-bottom:8px"></div>
      <div id="wifi-add-wrap">
        <div class="setting-desc" style="margin-bottom:6px"><b>Add network</b> <span id="wifi-slot-count" style="color:var(--tx3)">(0/4)</span></div>
        <div style="display:flex;gap:6px;margin-bottom:6px">
          <input class="sniff-input" id="wifi-ssid" placeholder="WiFi SSID" style="flex:1">
          <button class="sniff-btn" onclick="scanWifi()" id="scan-btn">Scan</button>
        </div>
        <div id="wifi-nets" style="display:none;margin-bottom:6px;max-height:140px;overflow-y:auto;border:1px solid var(--bd);border-radius:6px;background:var(--bg2)"></div>
        <div style="display:flex;gap:6px;margin-bottom:6px">
          <input class="sniff-input" id="wifi-pass" placeholder="Password" type="password" style="flex:1">
          <button class="sniff-btn" onclick="saveWifi()" id="wifi-save-btn">Save &amp; Connect</button>
        </div>
        <details style="margin-top:4px">
          <summary style="font-size:11px;color:var(--acc);cursor:pointer;user-select:none">Static IP (optional) <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Set a fixed IP configuration instead of using DHCP.">i</span></summary>
          <div style="margin-top:6px">
            <label style="font-size:11px;color:var(--tx3);display:flex;align-items:center;gap:6px;margin-bottom:6px">
              <input type="checkbox" id="wifi-static" onchange="toggleStaticIP()"> Use static IP
            </label>
            <div id="static-fields" style="display:none">
              <div style="display:grid;grid-template-columns:1fr 1fr;gap:4px">
                <input class="sniff-input" id="wifi-ip" placeholder="IP (e.g. 192.168.1.100)">
                <input class="sniff-input" id="wifi-gw" placeholder="Gateway (e.g. 192.168.1.1)">
                <input class="sniff-input" id="wifi-mask" placeholder="Mask (255.255.255.0)" value="255.255.255.0">
                <input class="sniff-input" id="wifi-dns" placeholder="DNS (e.g. 8.8.8.8)">
              </div>
            </div>
          </div>
        </details>
        <input type="hidden" id="wifi-edit-idx" value="-1">
      </div>
    </div>
  </div>

  <div class="subsec" data-subkey="config-gateway">
    <div class="subsec-head">
      <div class="subsec-title">STA-AP Gateway <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Routes hotspot clients through the configured WiFi Internet uplink, with DNS filtering.">i</span></div>
      <div class="subsec-meta"><span id="gw-status">Gateway status unavailable</span></div>
    </div>
    <div class="subsec-body">
      <div class="setting-row" style="padding:8px 0">
        <div class="setting-info">
          <div class="setting-name">Gateway</div>
          <div class="setting-desc">Enable STA-AP NAT routing for hotspot clients when WiFi Internet is connected</div>
        </div>
        <label class="tgl"><input type="checkbox" id="gw-enabled" onchange="saveGatewayDns()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div class="setting-row" style="padding:8px 0">
        <div class="setting-info">
          <div class="setting-name">Network Performance Mode</div>
          <div class="setting-desc">Reduce WebUI polling while AP+STA+NAPT is forwarding traffic</div>
          <div id="net-perf-status" style="font-size:10px;color:var(--tx3);margin-top:3px">ON: status 5s, network diagnostics 30s, heavy lists manual only</div>
        </div>
        <label class="tgl"><input type="checkbox" id="net-perf-tgl" onchange="setNetworkPerformanceMode(this.checked,true)"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div id="gw-diag" style="margin:2px 0 10px;padding:8px;border:1px solid var(--bd);border-radius:8px;background:var(--bg2);display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:6px;font-size:11px">
        <div><span style="color:var(--tx3)">AP</span> <span id="gw-diag-ap">--</span></div>
        <div><span style="color:var(--tx3)">STA</span> <span id="gw-diag-sta">--</span></div>
        <div><span style="color:var(--tx3)">NAT</span> <span id="gw-diag-nat">--</span></div>
        <div><span style="color:var(--tx3)">Radio</span> <span id="gw-diag-radio">--</span></div>
        <div><span style="color:var(--tx3)">DNS</span> <span id="gw-diag-dns">--</span></div>
        <div><span style="color:var(--tx3)">DNS Slow</span> <span id="gw-diag-slow">--</span></div>
        <div><span style="color:var(--tx3)">Pending</span> <span id="gw-diag-pending">--</span></div>
        <div><span style="color:var(--tx3)">Upstream</span> <span id="gw-diag-upstream">--</span></div>
        <div><span style="color:var(--tx3)">AP Clients</span> <span id="gw-diag-clients">--</span></div>
      </div>
      <div style="margin:4px 0 10px;padding:8px;border:1px solid var(--bd);border-radius:8px;background:var(--bg2)">
        <div style="font-size:12px;font-weight:600;color:var(--tx2);margin-bottom:6px">Upstream DNS</div>
        <input type="hidden" id="gw-upstream-mode" value="0">
        <div style="display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:6px;margin-bottom:6px">
          <button type="button" class="sniff-btn gateway-upstream-btn" data-mode="0" onclick="setGatewayUpstreamMode(0,true)">Auto</button>
          <button type="button" class="sniff-btn gateway-upstream-btn" data-mode="1" onclick="setGatewayUpstreamMode(1,true)">223.5.5.5 Ali</button>
          <button type="button" class="sniff-btn gateway-upstream-btn" data-mode="2" onclick="setGatewayUpstreamMode(2,true)">119.29.29.29 Tencent</button>
          <button type="button" class="sniff-btn gateway-upstream-btn" data-mode="3" onclick="setGatewayUpstreamMode(3,true)">Custom</button>
        </div>
        <div style="display:grid;grid-template-columns:minmax(0,1fr) auto auto;gap:6px;align-items:center">
          <input class="sniff-input" id="gw-upstream-custom" placeholder="Custom DNS, e.g. 8.8.8.8">
          <button class="sniff-btn modal-btn-primary" onclick="saveGatewayDns()">Save DNS</button>
          <button class="sniff-btn" onclick="resetGatewayDnsStats()">Reset DNS Stats</button>
        </div>
        <div id="gw-upstream-hint" style="font-size:10px;color:var(--tx3);margin-top:5px">Auto uses DHCP DNS from the connected WiFi; public DNS can avoid stale slow/fail counters from a bad router DNS.</div>
      </div>
      <div style="margin:4px 0 10px;padding:8px;border:1px solid var(--bd);border-radius:8px;background:var(--bg2)">
        <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px;margin-bottom:6px">
          <button class="sniff-btn gateway-profile-btn" id="gw-profile-safe" onclick="applyGatewayProfile('safe')">Conservative Mode</button>
          <button class="sniff-btn gateway-profile-btn" id="gw-profile-aggressive" onclick="applyGatewayProfile('aggressive')">Aggressive Mode</button>
        </div>
        <div id="gw-profile-desc" style="font-size:11px;color:var(--tx3);line-height:1.45">Conservative Mode: WiFi access / offline navigation / online navigation / China maps / WeChat notifications / Bluetooth music / voice assistant.</div>
      </div>
      <div style="margin-bottom:10px">
        <div style="font-size:12px;font-weight:600;color:var(--tx2);margin-bottom:4px">Blacklist</div>
        <textarea class="sniff-input" id="gw-blacklist" rows="5" placeholder="Blocked domains, one per line" style="width:100%;resize:vertical"></textarea>
      </div>
      <div style="margin-bottom:10px">
        <div style="font-size:12px;font-weight:600;color:var(--tx2);margin-bottom:4px">Whitelist</div>
        <textarea class="sniff-input" id="gw-whitelist" rows="5" placeholder="Allowed domains, one per line" style="width:100%;resize:vertical"></textarea>
      </div>
      <div style="display:flex;gap:6px;align-items:center;flex-wrap:wrap;margin-bottom:10px">
        <button class="sniff-btn modal-btn-primary" onclick="saveGatewayDns()">Save DNS</button>
        <span style="font-size:11px;color:var(--tx3)" id="gw-msg"></span>
        <span id="gw-list-counts" style="font-size:11px;color:var(--tx3);margin-left:auto"></span>
      </div>
      <div style="margin-bottom:10px">
        <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:4px">
          <div style="font-size:12px;font-weight:600;color:var(--tx2)">Filter List</div>
          <div style="display:flex;gap:6px">
            <button class="sniff-btn" onclick="loadGatewayBlocked()" style="padding:3px 8px;font-size:10px">Refresh</button>
            <button class="sniff-btn" onclick="clearGatewayBlocked()" style="padding:3px 8px;font-size:10px">Clear</button>
          </div>
        </div>
        <div id="gw-blocked-list" class="dns-modal-list" style="margin-top:0;max-height:240px"></div>
        <div id="gw-blocked-summary" style="font-size:10px;color:var(--tx3);margin-top:4px"></div>
        <div id="gw-blocked-msg" style="font-size:10px;color:var(--tx3);margin-top:2px"></div>
      </div>
      <div style="display:flex;gap:6px;margin-bottom:6px">
        <input class="sniff-input" id="gw-test-domain" placeholder="Test domain">
        <button class="sniff-btn" onclick="testGatewayDns()">Test DNS</button>
      </div>
      <div id="gw-test-result" style="font-size:11px;color:var(--tx3);margin-bottom:8px"></div>
      <div style="display:flex;gap:6px;align-items:center;flex-wrap:wrap">
        <button class="sniff-btn" onclick="saveGatewayDns()">Save DNS</button>
      </div>
    </div>
  </div>

  <div class="subsec can-debug-panel" data-subkey="config-can-pins">
    <div class="subsec-head">
      <div class="subsec-title">CAN Pins <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Set the ESP32 GPIO pins used for the CAN transceiver. Wrong values can disable CAN.">i</span></div>
      <div class="subsec-meta" id="can-pins-status">default</div>
    </div>
    <div class="subsec-body">
      <div style="display:flex;gap:6px;align-items:center">
        <input class="sniff-input" id="can-tx" type="number" min="0" max="39" placeholder="TX GPIO" style="flex:1">
        <input class="sniff-input" id="can-rx" type="number" min="0" max="39" placeholder="RX GPIO" style="flex:1">
        <button class="sniff-btn" onclick="saveCanPins()">Save</button>
      </div>
      <div style="font-size:11px;color:var(--tx3);margin-top:6px" id="can-pins-hint">Reboot required after change</div>
    </div>
  </div>

  <div class="subsec can-debug-panel" data-subkey="config-dashboard-log" style="margin-top:14px">
    <div class="subsec-head">
      <div class="subsec-title">Debug Log <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Shows recent WebUI and firmware log lines. This is debug logging output, not the CAN sniffer.">i</span></div>
      <div class="subsec-meta">Recent debug output</div>
    </div>
    <div class="subsec-body">
      <div class="setting-row" style="padding-top:0">
        <div class="setting-info">
          <div class="setting-name">Debug logging <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Turns WebUI debug log output on or off.">i</span></div>
          <div class="setting-desc">Toggle WebUI and firmware debug output</div>
        </div>
        <label class="tgl"><input type="checkbox" id="tgl-eprn" checked onchange="pushLogging()">
          <div class="tgl-track"><div class="tgl-thumb"></div></div></label>
      </div>
      <div class="log-box" id="log">Waiting...</div>
    </div>
  </div>
  <div class="setting-row" style="margin-top:14px;padding-top:12px;border-top:1px solid var(--bd)">
    <div class="setting-info">
      <div class="setting-name">Settings Backup <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" data-help-target="backup-info" title="Export or restore saved device settings as JSON.">i</span></div>
      <div class="setting-desc">Export and import device settings</div>
    </div>
    <button class="sniff-btn" onclick="exportSettings()">Download</button>
    <button class="sniff-btn" onclick="document.getElementById('backup-file').click()">Upload &amp; Restore</button>
    <input type="file" id="backup-file" accept=".json,application/json" style="display:none" onchange="importSettings(event)">
    <span style="font-size:11px;color:var(--tx3)" id="backup-status"></span>
  </div>
  <div class="setting-row can-debug-panel" style="padding-top:12px;border-top:1px solid var(--bd)">
    <div class="setting-info">
      <div class="setting-name">Support <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Collect a support summary and open a GitHub issue with the details prefilled.">i</span></div>
      <div class="setting-desc">Copy a status summary before opening a GitHub issue</div>
    </div>
    <button class="sniff-btn" onclick="openSupport()">Open</button>
  </div>
  <div id="backup-info" class="info-box" style="display:none">
    Exports AP credentials, WiFi Internet, CAN pins, HW3 speed settings and gateway DNS settings as JSON. Useful before a full re-flash or when migrating to another device. <b>Passwords are included in clear text</b> &mdash; keep the file safe.
  </div>
</div>

<div class="card can-debug-panel">
  <div class="card-hdr">
    <div class="card-title">Firmware Update <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Manual firmware upload only. Select a local .bin and flash it to the device.">i</span></div>
    <div class="card-meta" id="fw-ver">Manual OTA</div>
  </div>
  <div style="margin-top:4px">
    <div class="ota-drop" id="ota-drop" onclick="$('ota-file').click()" ondragover="event.preventDefault();this.classList.add('drag')" ondragleave="this.classList.remove('drag')" ondrop="handleDrop(event)">
      <input type="file" id="ota-file" accept=".bin" onchange="fileSelected(this.files[0])">
      <div class="ota-icon">&#8679;</div>
      <div class="ota-text">Tap to select firmware .bin</div>
      <div class="ota-sub">Or drag and drop a file here</div>
    </div>
    <div class="ota-progress" id="ota-progress">
      <div class="ota-bar"><div class="ota-fill" id="ota-fill"></div></div>
      <div class="ota-status" id="ota-status">Uploading...</div>
    </div>
    <button class="ota-btn" id="ota-upload-btn" onclick="uploadFirmware()">Flash Firmware</button>
    <button class="sniff-btn" id="ota-reset-btn" onclick="resetOtaCredentials()" style="width:100%;margin-top:6px">Reset OTA Credentials</button>
    <div style="margin-top:10px;font-size:11px;color:var(--tx3);line-height:1.7">
      Use the generated PlatformIO firmware.bin for this board.<br>
      Current build path: <span style="color:var(--acc);font-family:monospace">.pio/build/waveshare_ESP32_S3_RS485_CAN/firmware.bin</span>
    </div>
  </div>
</div>
<div class="card can-debug-panel">
  <div class="card-hdr"><div class="card-title">CAN <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Live CAN tools for sniffing, recording, controller status and checking the last injected write.">i</span></div><div class="card-meta">Sniffer, recorder and bus status</div></div>

  <div class="subsec" data-subkey="can-sniffer">
    <div class="subsec-head">
      <div class="subsec-title">CAN Sniffer <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Shows the latest 30 CAN frames live. You can filter by ID or name, switch between wire IDs and DBC IDs, and pause the view.">i</span></div>
      <div class="subsec-meta" id="sniff-count">0 frames</div>
    </div>
    <div class="subsec-body">
      <div class="sniff-ctrl">
        <input class="sniff-input" id="sniff-filter" placeholder="Filter by ID or name" oninput="renderSniffer()">
        <button class="sniff-btn" id="sniff-id-btn" onclick="toggleSniffIdMode()">Wire IDs</button>
        <button class="sniff-btn" id="sniff-pause-btn" onclick="togglePause()">Pause</button>
      </div>
      <div class="sniff-box" id="sniffer">
        <div style="padding:20px;color:var(--tx3);text-align:center;font-size:12px">Waiting for CAN frames</div>
      </div>
    </div>
  </div>

  <div class="subsec" data-subkey="can-recorder">
    <div class="subsec-head">
      <div class="subsec-title">CAN Recorder <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Records live CAN traffic up to the frame limit and lets you download it as a CSV file.">i</span></div>
      <div class="subsec-meta" id="rec-meta">Idle</div>
    </div>
    <div class="subsec-body">
      <div class="rec-bar"><div class="rec-fill" id="rec-fill"></div></div>
      <div class="rec-info">
        <span id="rec-count">0 / -- frames</span>
        <span id="rec-status">Ready</span>
      </div>
      <div class="btn-row">
        <button class="btn" id="rec-btn" onclick="toggleRec()">Start Recording</button>
        <a class="btn" id="rec-dl" href="/rec_download" download="can_recording.csv" style="display:none;text-align:center;text-decoration:none;padding:10px;border:1px solid var(--bd2);color:var(--tx2)">Download CSV</a>
      </div>
    </div>
  </div>

  <div class="subsec" data-subkey="can-controller">
    <div class="subsec-head">
      <div class="subsec-title">CAN Controller <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Shows CAN controller health, error flags and the RX, TX and error counters per mux.">i</span></div>
      <div class="subsec-meta" style="display:flex;align-items:center;gap:8px">
        <button onclick="resetStats()" style="font-size:10px;padding:2px 8px;border:1px solid var(--bd2);border-radius:5px;background:transparent;color:var(--tx3);cursor:pointer;font-family:inherit">Reset</button>
      </div>
    </div>
    <div class="subsec-body">
      <div class="eflg-row" id="eflg-row"><span class="eflg-pill eflg-ok">OK</span></div>
      <table class="mux-tbl">
        <tr><th>Mux</th><th>RX</th><th>TX</th><th>Errors</th></tr>
        <tr><td>0</td><td id="m0rx">0</td><td id="m0tx">0</td><td id="m0err">0</td></tr>
        <tr><td>1</td><td id="m1rx">0</td><td id="m1tx">0</td><td id="m1err">0</td></tr>
        <tr><td>2</td><td id="m2rx">0</td><td id="m2tx">0</td><td id="m2err">0</td></tr>
      </table>
    </div>
  </div>

  <div class="subsec" data-subkey="can-last-write-check">
    <div class="subsec-head">
      <div class="subsec-title">Last Write Check <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Compares the last injected frame with the latest bus frame that has the same CAN ID and mux. Helpful to spot overwrites, but not proof that a module accepted the change.">i</span></div>
    </div>
    <div class="subsec-body">
      <div class="probe-status v-dim" id="probe-status">No injected frame yet</div>
      <div class="probe-block">
        <div class="probe-label">Sent</div>
        <div class="probe-meta" id="probe-tx-meta">--</div>
        <div class="probe-hex" id="probe-tx">--</div>
      </div>
      <div class="probe-block">
        <div class="probe-label">Bus</div>
        <div class="probe-meta" id="probe-rx-meta">--</div>
        <div class="probe-hex" id="probe-rx">--</div>
      </div>
    </div>
  </div>
</div>

<div class="card" id="can-debug-card">
  <div class="card-hdr">
    <div class="card-title">CAN Debug <span class="title-help" aria-label="Help" onclick="return toggleHelp(this,event)" title="Enable debug panels: firmware update, debug log and live CAN tools.">i</span></div>
    <div class="card-meta" id="can-debug-meta">Off</div>
  </div>
  <div class="setting-row" style="padding-top:0">
    <div class="setting-info">
      <div class="setting-name">Enable CAN debug tools</div>
      <div class="setting-desc">Shows firmware update, logs, sniffer and recorder panels</div>
    </div>
    <label class="tgl"><input type="checkbox" id="can-debug-tgl" onchange="toggleCanDebug()"><div class="tgl-track"><div class="tgl-thumb"></div></div></label>
  </div>
</div>

<div class="warn-bar">CAN bus writes affect vehicle behavior. Remove device immediately if unexpected behavior occurs. Not affiliated with any vehicle manufacturer.</div>

<div class="modal-backdrop" id="owner-modal" onclick="ownerNoticeBackdrop(event)">
  <div class="modal-card owner-modal-card" role="dialog" aria-modal="true" aria-labelledby="owner-title">
    <div class="owner-modal-title" id="owner-title">&#x3010;FSD&#x8F66;&#x4E3B;&#x4EA4;&#x6D41;&#x7FA4;&#x3011;</div>
    <div class="owner-modal-subtitle">&#x836F;&#x4E0D;&#x80FD;&#x505C; &#x4E13;&#x5C5E;&#x5FAE;&#x96EA;ESP32S3&#x56FA;&#x4EF6;</div>
    <div class="owner-modal-msg">&#x57FA;&#x4E8E;&#x5B98;&#x65B9;3.0.0 &#x5B8C;&#x6574; IDF&#x6846;&#x67B6;&#x79FB;&#x690D;
&#x6DF1;&#x5EA6;&#x4F18;&#x5316;&#xFF1A;
<span class="ok">&#x2705;</span> WiFi AP + NAPT &#x8F6C;&#x53D1;&#x901F;&#x5EA6;
<span class="ok">&#x2705;</span> DNS &#x8FC7;&#x6EE4;&#x4E0E;&#x89E3;&#x6790;&#x6548;&#x7387;
<span class="ok">&#x2705;</span> &#x81EA;&#x5B9A;&#x4E49;&#x9650;&#x901F;

Version: 3.0.0-beta.5
OTA timestamp: 2026-05-23 21:47:24 +08:00</div>
    <div class="modal-actions">
      <button class="sniff-btn modal-btn-primary" onclick="closeOwnerNotice()">&#x77E5;&#x9053;&#x4E86;</button>
    </div>
  </div>
</div>

<div class="modal-backdrop" id="confirm-modal" onclick="dashConfirmBackdrop(event)">
  <div class="modal-card" role="dialog" aria-modal="true" aria-labelledby="confirm-title">
    <div class="modal-title" id="confirm-title">Confirm</div>
    <div class="modal-msg" id="confirm-msg"></div>
    <div class="modal-actions">
      <button class="sniff-btn" id="confirm-cancel" onclick="dashConfirmResolve(false)">Cancel</button>
      <button class="sniff-btn modal-btn-primary" id="confirm-ok" onclick="dashConfirmResolve(true)">Continue</button>
    </div>
  </div>
</div>

<div class="modal-backdrop" id="ota-test-modal" onclick="otaTestBackdrop(event)">
  <div class="modal-card" role="dialog" aria-modal="true" aria-labelledby="ota-test-title">
    <div class="modal-title" id="ota-test-title">OTA Test v2</div>
    <div class="modal-msg" id="ota-test-msg">Version: 3.0.0-beta.5
OTA timestamp: 2026-05-23 21:47:24 +08:00</div>
    <div class="modal-actions">
      <button class="sniff-btn modal-btn-primary" onclick="closeOtaTestNotice()">Close</button>
    </div>
  </div>
</div>

<div class="modal-backdrop" id="support-modal" onclick="supportBackdrop(event)">
  <div class="modal-card" role="dialog" aria-modal="true" aria-labelledby="support-title" style="width:min(100%,560px)">
    <div class="modal-title" id="support-title">Support</div>
    <div class="modal-msg" style="margin-top:10px">
      <textarea id="support-body" readonly style="width:100%;min-height:260px;resize:vertical;border:1px solid var(--bd2);border-radius:8px;background:var(--bg);color:var(--tx);padding:10px;font:inherit;line-height:1.5"></textarea>
    </div>
    <div class="modal-actions" style="justify-content:space-between;align-items:center">
      <span id="support-status" style="font-size:11px;color:var(--tx3)"></span>
      <div style="display:flex;gap:8px;flex-wrap:wrap;justify-content:flex-end">
        <button class="sniff-btn" onclick="copySupport()">Copy</button>
        <button class="sniff-btn modal-btn-primary" onclick="openSupportIssue()">Open GitHub Issue</button>
        <button class="sniff-btn" onclick="closeSupport()">Close</button>
      </div>
    </div>
  </div>
</div>


<div class="foot" id="dash-foot"ev-open-can-tools &bull; loading...</div>
<div class="foot" style="margin-top:4px">
  <a href="https://github.com/ev-open-can-tools/ev-open-can-tools" target="_blank" rel="noopener" style="color:var(--acc);text-decoration:none">GitHub</a>
  &bull;
  <a href="https://discord.gg/ZTQKAUTd2F" target="_blank" rel="noopener" style="color:var(--acc);text-decoration:none">Discord</a>
</div>
<div class="foot" style="margin-top:8px;font-size:10px">
  <div style="margin-bottom:4px">Gift with Monero</div>
  <div style="word-break:break-all;color:var(--tx2)">46CJEjnN74N83AZHHYKX3mD9kkV6UJYVjN58PTWvQ6VU8Vvn3tmyExkaC2kq9asD6SZY9weaZqx5o9nf1MxkKbmTKWLUeRD</div>
</div>

<script>
const HW=['Legacy','HW3','HW4'];
const SP3=['Chill','Normal','Hurry'];
const SP4=['Chill','Normal','Hurry','Max','Sloth'];
const $=id=>document.getElementById(id);
let dashLang=localStorage.getItem('dashLang')||((navigator.language||'').toLowerCase().startsWith('zh')?'zh':'en');
const I18N_ZH={
'Light':'浅色','Dark':'深色','Waiting for CAN frames':'等待 CAN 帧','Dashboard disconnected':'仪表盘已断开','Dashboard reconnecting':'仪表盘重连中',
'CAN Bus':'CAN 总线','Injection':'注入','Frame rate':'CAN 帧率','CAN Frame Rate':'CAN 帧率','RX Frames':'接收帧','TX Frames':'发送帧','Errors':'错误','AD Status':'AP 状态','Profile':'配置档','Offset':'偏移','Uptime':'运行时间',
'Offline':'离线','Online':'在线','Active':'运行中','Inactive':'未激活','BLOCKED':'已阻止','Waiting AP':'等待 AP','No frames':'无帧','Sniffer paused':'嗅探暂停',
'WiFi Hotspot':'WiFi 热点','Change the WiFi hotspot name and password':'修改 WiFi 热点名称和密码','SSID':'SSID','Password':'密码','Hidden':'隐藏','WiFi Internet':'WiFi 互联网','Not configured':'未配置','Save up to 4 networks (e.g. home + phone hotspot).':'最多保存 4 个网络（如家庭 WiFi + 手机热点）。','Add network':'添加网络','WiFi SSID':'WiFi SSID','Scan':'扫描','Save & Connect':'保存并连接','Use static IP':'使用静态 IP',
'STA-AP Gateway':'STA-AP 网关','Gateway status unavailable':'网关状态不可用','Gateway':'网关','Enable STA-AP NAT routing for hotspot clients when WiFi Internet is connected.':'WiFi 互联网连接后，为热点客户端启用 STA-AP NAT 路由。','Conservative Mode':'保守模式','Aggressive Mode':'激进模式','Conservative Mode: WiFi access / offline navigation / online navigation / China maps / WeChat notifications / Bluetooth music / voice assistant.':'保守模式：WiFi 接入 / 离线导航 / 在线导航 / 中国地图 / 微信通知 / 蓝牙音乐 / 车机语音助手。','Aggressive Mode: WiFi access / offline navigation / online navigation / China maps / WeChat notifications / Bluetooth music / voice assistant / app vehicle control.':'激进模式：WiFi 接入 / 离线导航 / 在线导航 / 中国地图 / 微信通知 / 蓝牙音乐 / 车机语音助手 / 控车。','Custom DNS profile':'自定义 DNS 配置','Blacklist':'黑名单','Whitelist':'白名单','Save DNS':'保存 DNS','Blocked':'阻断记录','DNS Filter List':'DNS 过滤清单','Add to Whitelist':'加入白名单','Blacklisted':'黑名单','Already whitelisted':'已在白名单','Clear':'清空','No blocked domains recorded':'没有阻断记录','Cleared':'已清空','Gateway not available':'网关不可用','domain is blacklisted':'域名在黑名单中，禁止加入白名单','cannot add domain':'无法加入域名',
'CAN Pins':'CAN 引脚','default':'默认','TX GPIO':'TX GPIO','RX GPIO':'RX GPIO','Reboot required after saving custom pins.':'保存自定义引脚后需要重启。','Dashboard Log':'调试日志','Debug Log':'调试日志','Settings Backup':'设置备份','Export and import device settings':'导出和导入设备设置','Download':'下载','Import':'导入','Support':'支持','Open':'打开',
'Firmware Update':'固件更新','Beta Channel':'Beta 通道','Include pre-release / beta firmware versions':'包含预发布 / beta 固件版本','Auto-Update on Boot':'启动后自动更新','Check and install updates automatically ~15 s after WiFi connects':'WiFi 连接约 15 秒后自动检查并安装更新','Check for Updates':'检查更新','Manual firmware upload':'手动上传固件','Tap to select firmware .bin':'点击选择固件 .bin','Or drag and drop a file here':'或将文件拖放到这里','Uploading...':'上传中...','Flash Firmware':'刷写固件','Reset OTA Credentials':'重置 OTA 凭据',
  'System Health':'系统状态','System Status':'系统状态','Hardware and runtime health reported by the ESP32 firmware.':'ESP32 固件上报的硬件与运行状态。','CAN Debug':'CAN 调试','CAN调试':'CAN 调试','Enable CAN debug tools':'启用 CAN 调试工具','Shows firmware update, logs, sniffer and recorder panels':'显示固件更新、日志、嗅探器和记录器面板','Chip':'芯片','CPU':'CPU','CPU Load':'CPU 负载','Task Load':'任务负载','task':'任务','core':'核心','cpu%':'CPU%','stack':'栈余量','state':'状态','Task stats unavailable':'任务负载不可用','Core 0':'核心 0','Core 1':'核心 1','Board Specs':'板载规格','Temperature':'温度','Reset':'重启原因','Heap RAM':'堆内存','Largest Block':'最大连续内存块','Min Free Heap':'历史最低空闲内存','PSRAM':'PSRAM','Tasks':'任务','Flash':'Flash','SPIFFS':'SPIFFS','WiFi RSSI':'WiFi 信号','WiFi Mode':'WiFi 模式','AP Clients':'AP 客户端','Bluetooth LE':'蓝牙 LE','Wireless':'无线','MAC / Firmware':'MAC / 固件','System status unavailable':'系统状态不可用','Monitoring off':'监测关闭','Enable live hardware status sampling':'启用实时硬件状态采样','On':'开启','Off':'关闭','off':'关闭','not enabled':'未启用','unavailable':'不可用','offline':'离线','not present':'不存在','STA online':'STA 在线','STA offline':'STA 离线','supported':'支持','not supported':'不支持','firmware disabled':'固件未启用','warming up':'采样中',
'CAN':'CAN','CAN Sniffer':'CAN 嗅探器','Pause':'暂停','Resume':'继续','Wire IDs':'线束 ID','CAN Recorder':'CAN 记录器','Start Recording':'开始记录','Stop Recording':'停止记录','Ready':'就绪','Saved':'已保存','Recording...':'记录中...','CAN Controller':'CAN 控制器','Last Write Check':'最后写入检查','Reset Stats':'重置统计',
'Cancel':'取消','Continue':'继续','Confirm':'确认','Copy':'复制','Open GitHub Issue':'打开 GitHub Issue','Close':'关闭','Show':'显示','Hide':'隐藏','Loading...':'加载中...','Saving...':'保存中...','Saved! Reboot to apply.':'已保存！重启后生效。','Saved':'已保存','Error':'错误','Save failed':'保存失败','Connection error':'连接错误','Connection to ':'到 ',
'Enabled':'已启用','Disabled':'已禁用','on':'开启','waiting':'等待中','blocked':'已阻断','NAT':'NAT','Connected':'已连接','Connecting to ':'正在连接 ','Connect':'连接','Reconnect':'重连','Connect failed':'连接失败','Delete':'删除','Edit':'编辑','No networks saved.':'未保存网络。','firmware default':'固件默认','saved':'已保存'
};
Object.assign(I18N_ZH,{
  'Configuration':'\u914d\u7f6e',
  'CONFIGURATION':'\u914d\u7f6e',
  'Hardware':'\u786c\u4ef6',
  'Device settings':'\u8bbe\u5907\u8bbe\u7f6e',
  'Autopilot generation':'Autopilot \u4ee3\u9645',
  'Select the autopilot hardware generation and matching speed profile set.':'\u9009\u62e9 Autopilot \u786c\u4ef6\u4ee3\u9645\u548c\u5bf9\u5e94\u7684\u901f\u5ea6\u914d\u7f6e\u6863\u3002',
  'Configure the device hotspot name, password and visibility. Saved in NVS.':'配置设备热点名称、密码和可见性，保存到 NVS。',
  'Stored in NVS (non-volatile storage). The SSID and password survive firmware updates and reboots. Only a full factory erase via USB clears them.':'保存在 NVS 非易失存储中。SSID 和密码会在固件更新、重启后保留，只有通过 USB 完整擦除出厂设置才会清除。',
  'Hotspot Name':'热点名称',
  'New Password (min 8)':'新密码（至少 8 位）',
  'Hide SSID':'隐藏 SSID',
  "Don't broadcast the hotspot name — clients must enter it manually":'不广播热点名称，客户端需要手动输入。',
  'Changes take effect after reboot. Leave password empty to keep current.':'更改将在重启后生效。密码留空表示保留当前密码。',
  'Up to 4 saved networks. The device tries each in turn until one connects.':'最多保存 4 个网络，设备会依次尝试直到连接成功。',
  'Save up to 4 networks (e.g. home + phone hotspot). Device tries each in turn. Stored in NVS — survives firmware updates.':'最多保存 4 个网络（如家庭 WiFi + 手机热点）。设备会依次尝试连接，并保存在 NVS 中，固件更新后仍会保留。',
  'Static IP (optional)':'静态 IP（可选）',
  'Set a fixed IP configuration instead of using DHCP.':'使用固定 IP 配置，而不是 DHCP 自动获取。',
  'IP (e.g. 192.168.1.100)':'IP（例如 192.168.1.100）',
  'Gateway (e.g. 192.168.1.1)':'网关（例如 192.168.1.1）',
  'Mask (255.255.255.0)':'掩码（255.255.255.0）',
  'DNS (e.g. 8.8.8.8)':'DNS（例如 8.8.8.8）',
  'Routes hotspot clients through the configured WiFi Internet uplink, with DNS filtering.':'通过已配置的 WiFi 互联网连接为热点客户端转发网络，并执行 DNS 过滤。',
  'Conservative Mode':'保守模式',
  'Aggressive Mode':'激进模式',
  'Conservative Mode: WiFi access / offline navigation / online navigation / China maps / WeChat notifications / Bluetooth music / voice assistant.':'保守模式：WiFi 接入 / 离线导航 / 在线导航 / 中国地图 / 微信通知 / 蓝牙音乐 / 车机语音助手。',
  'Aggressive Mode: WiFi access / offline navigation / online navigation / China maps / WeChat notifications / Bluetooth music / voice assistant / app vehicle control.':'激进模式：WiFi 接入 / 离线导航 / 在线导航 / 中国地图 / 微信通知 / 蓝牙音乐 / 车机语音助手 / 控车。',
  'Custom DNS profile':'自定义 DNS 配置',
  'Enable STA-AP NAT routing for hotspot clients when WiFi Internet is connected':'WiFi 互联网连接后，为热点客户端启用 STA-AP NAT 路由。',
  'Blocked domains, one per line':'阻止域名，每行一个',
  'Allowed domains, one per line':'允许域名，每行一个',
  'Test domain':'测试域名',
  'Test DNS':'测试 DNS',
  'DNS test failed':'DNS 测试失败',
  'would be blocked':'会被阻断',
  'would be allowed':'会被放行',
  'matched blacklist':'命中黑名单',
  'not in blacklist':'不在黑名单中',
  'matched whitelist':'命中白名单',
  'not in whitelist':'不在白名单中',
  'gateway disabled':'网关未启用',
  'empty domain':'域名为空',
  'Filtered DNS Entries':'DNS 过滤记录',
  'items':'条',
  'Only non-blacklist domains can be added to the whitelist.':'只有非黑名单域名可以加入白名单。',
  'Blacklist blocked':'黑名单禁止加入白名单',
  'Not allowed':'不可加入',
  'DNS filter list unavailable':'DNS 过滤记录不可用'
  ,'Enter hotspot name':'请输入热点名称'
  ,'Password min 8 chars':'密码至少 8 位'
  ,'Scanning...':'扫描中...'
  ,'Save Changes':'保存更改'
});
Object.assign(I18N_ZH,{
  // Core terminology refinements (Tesla FSD / CAN context)
  'Stop Injection':'停止 CAN 注入','Resume Injection':'恢复 CAN 注入','Stop Injecting':'停止 CAN 注入',
  'FSD Switch':'FSD 开关','Turn FSD Off':'FSD 关闭','Turn FSD On':'开启 FSD',
  'FSD Master Switch':'FSD 总开关','Enable FSD activation':'启用 FSD 激活',
  'Turns built-in Legacy/HW3/HW4 FSD activation and CAN injection on or off together.':'同时开启或关闭内置 HW3 FSD 激活链路和 CAN 注入。',
  'ON = built-in Legacy/HW3/HW4 FSD activation and CAN injection are enabled together.':'开启 = 使用内置 Legacy/HW3/HW4 FSD 激活链路，并同时允许 CAN 注入。',
  'Built-in FSD chain is active. Legacy/HW3/HW4 injection is controlled by this switch.':'内置 FSD 链路已启用。Legacy/HW3/HW4 注入都由此开关统一控制。',
  'FSD chain and CAN injection are disabled and stay off after reboot.':'FSD 链路和 CAN 注入已关闭，重启后也保持关闭。',
  'FSD chain and CAN injection are disabled.':'FSD 链路和 CAN 注入已关闭。',
  'Built-in FSD chain is active.':'内置 FSD 链路已启用。',
  'Turn FSD off':'FSD 关闭','Turn Off':'关闭',
  'Heap RAM':'堆内存','Largest Block':'最大连续内存块','Min Free Heap':'历史最低空闲内存',
  // Header / status
  'Reboot':'重启设备','RX':'接收','TX':'发送','TX Errors':'发送错误','Follow dist':'跟车距离','Speed Offset':'限速偏移','Limit Offset':'限速偏移',
  'AD active — injecting':'AP 已激活 — CAN 注入运行中',
  'AP active — injecting':'AP 已激活 — CAN 注入运行中',
  'FSD requested — injecting':'FSD 已请求 — CAN 注入运行中',
  'CAN active — injecting':'CAN 在线 — 注入运行中',
  'Waiting for AP — injection armed':'等待 AP — 注入待命',
  'CAN active — monitoring':'CAN 在线 — 监听中',
  'Stop injecting? This remains disabled after reboot until you press Resume Injection.':'停止 CAN 注入？该状态在重启后仍保持，直到按下"恢复 CAN 注入"。',
  'Stop injection':'停止 CAN 注入','Stop':'停止',
  'Reboot device?':'重启设备？',
  // System Health card
  'cores':'核',
  'Frames received per second':'每秒接收 CAN 帧数',
  'GTW 2047 Replay Count':'GTW 2047 重放次数',
  'Modified GTW_autopilot frames sent per observed 0x7FF frame':'每观察到一帧 0x7FF 后发送的 GTW_autopilot 修改帧数',
  'Author (optional)':'作者（可选）','Version':'版本',
  'Fast builder':'快速构建','Add Shortcut':'添加快捷规则',
  
  
  
  
  'No rules':'无规则',
  'Add Rule':'添加规则','+ Add Rule':'+ 添加规则','Remove Rule':'删除规则',
  'Idle':'空闲',
  'Download JSON':'下载 JSON','JSON Preview':'JSON 预览','Reset':'重置','Author':'作者',
  // Configuration card
  'Device settings for hardware mode, WiFi, CAN pins, logging and backup.':'设备的硬件模式、WiFi、CAN 引脚、日志与备份设置。',
  'Select the autopilot hardware generation and matching speed profile set.':'选择 Autopilot 硬件代际和对应的速度配置档。',
  'Autopilot generation':'Autopilot 代际',
  'Available profiles depend on the selected hardware.':'可用配置档取决于所选硬件。',
  'Auto follows the vehicle follow distance.':'自动模式跟随车辆跟车距离。',
  'Manual SP3 profile is locked.':'已锁定手动 SP3 配置档。',
  'Manual SP4 profile is locked.':'已锁定手动 SP4 配置档。',
  'Profiles are only available on HW3 and HW4.':'仅 HW3 与 HW4 支持配置档。',
  // HW3 Custom Speed
  'HW3 Custom Speed Limit':'HW3 自定义限速',
  'Legacy Custom Speed Limit':'Legacy 自定义限速',
  'Raise UI_mppSpeedLimit on CAN 760 byte 6 to a target km/h based on what the gateway is currently sending. Same bucket layout as HW3. Only writes when target is higher than current - never lowers.':'根据网关当前发送的 UI_mppSpeedLimit (CAN 760 byte 6) 按桶查表得到目标 km/h，仅在目标值高于当前值时写回，从不降低。桶布局与 HW3 一致。',
  'Master enable':'总开关',
  'Allow this module to write to UI_mppSpeedLimit':'允许此模块写入 UI_mppSpeedLimit',
  'Bus raw':'总线原始值',
  'Sent raw':'写入原始值',
  'Override AP fused speed limit by writing a synthetic offset into 1021 mux 2. Custom uses the per-bucket table; High-speed uses target km/h values above 80 km/h.':'通过向 1021 mux 2 写入合成偏移来覆盖 AP 融合限速。Custom 使用分段表；High-speed 处理 80 km/h 以上的目标速度。',
  'Custom table':'自定义分段表',
  '30/40/50/60/70 km/h buckets':'30/40/50/60/70 km/h 分段',
  'High-speed boost (>=80 km/h)':'??????>=80 km/h?',
  '80/100/120 km/h target speeds':'80/100/120 km/h 目标速度',
  'Wire encoding':'报文编码',
  'PCT4=current, KPH5=legacy fleets':'PCT4=当前编码，KPH5=旧版车队',
  'Fused':'融合限速','Stock off':'原车限速偏移','Stock limit offset':'原车限速偏移','Tgt raw':'写入 raw','Write raw':'写入 raw',
  // HW3 slew
  'Limits downward HW3 mux 2 offset changes sent by the built-in FSD chain.':'限制内置 HW3 mux2 偏移下调速度，避免限速突然下降。',
  'Ramp-down limiter':'下行限速器',
  'Opt-in only; increases still pass immediately':'仅手动开启，向上调整仍立即生效',
  'Slew Rate':'限速偏移下降速率','Offset drop rate':'限速偏移下降速率','Target':'目标','Last':'上次','Capped':'触发限制',
  // AP Injection Gate
  'Start after AP':'AP 激活后启动',
  // WiFi
  'No networks saved.':'未保存网络。','No networks found':'未找到网络',
  'Edit':'编辑','Delete':'删除','Delete WiFi':'删除 WiFi',
  'Leave empty to keep current':'留空表示保留当前密码',
  'Password required':'请输入密码','Enter SSID':'请输入 SSID','Scan failed':'扫描失败',
  // STA-AP Gateway
  'Routes hotspot clients through the configured WiFi Internet uplink, with DNS filtering.':'通过已配置的 WiFi 互联网上行为热点客户端转发，并提供 DNS 过滤。',
  'Filter List':'过滤清单','Clear':'清空','Refresh':'刷新',
  'Current: Blacklist mode - saving automatically':'当前：黑名单模式 - 自动保存中',
  'Current: Blacklist mode - click mode to save immediately':'当前：黑名单模式 - 点击模式立即保存',
  'Current: Blacklist mode - saved':'当前：黑名单模式 - 已保存',
  'Already in blacklist':'已在黑名单',
  'Gateway not available':'网关不可用',
  // CAN Pins
  'Save CAN pins':'保存 CAN 引脚',
  'Reboot required after change':'修改后需要重启',
  'Enter both TX and RX':'请同时填写 TX 和 RX',
  'Saved. Rebooting...':'已保存，重启中...',
  // Dashboard log
  'Recent dashboard output':'最近调试输出',
  'Recent debug output':'最近调试输出',
  'Shows recent dashboard and firmware log lines. This is the dashboard logging output, not the CAN sniffer.':'显示最近 WebUI 与固件日志；这不是 CAN 嗅探器。',
  'Shows recent WebUI and firmware log lines. This is debug logging output, not the CAN sniffer.':'显示最近 WebUI 与固件日志；这不是 CAN 嗅探器。',
  'Turns dashboard log output on or off.':'开启或关闭 WebUI 与固件调试日志输出。',
  'Turns WebUI debug log output on or off.':'开启或关闭 WebUI 与固件调试日志输出。',
  'Dashboard Logging':'调试日志开关',
  'Debug logging':'调试日志开关',
  'Toggle dashboard log output':'开启或关闭 WebUI 与固件调试日志输出',
  'Toggle WebUI and firmware debug output':'开启或关闭 WebUI 与固件调试日志输出',
  'Waiting...':'等待中...','Loading...':'加载中...','Saving...':'保存中...','Checking...':'检查中...',
  'Downloading...':'下载中...','Uploading...':'上传中...','Installing...':'安装中...',
  'Preparing...':'准备中...','Downloaded':'已下载','Installed':'已安装',
  // Settings backup
  'Export or restore saved device settings as JSON.':'导出或还原已保存的设备设置（JSON 格式）。',
  'Upload & Restore':'上传并还原',
  'Restore settings':'还原设置','Restore':'还原',
  'Export failed':'导出失败','Invalid JSON':'JSON 格式错误',
  'Restored. Rebooting...':'已还原，重启中...','Import failed':'导入失败','Upload failed':'上传失败',
  // Support
  'Copy a status summary before opening a GitHub issue':'在提交 GitHub 问题前复制一份状态摘要',
  'Collect a support summary and open a GitHub issue with the details prefilled.':'收集支持信息摘要，并以预填详情打开 GitHub 问题。',
  'Copy this text, then open the GitHub issue form.':'复制下方文本后再打开 GitHub 问题表单。',
  'Copied to clipboard':'已复制到剪贴板','Copy failed':'复制失败',
  'Copied support details. Paste them into the support question.':'已复制支持信息，请粘贴到问题描述中。',
  // Firmware update
  'Version info':'版本信息',
  'Check for updates, enable beta builds and upload firmware manually.':'检查更新、启用 Beta 构建或手动上传固件。',
  'Shows pre-release firmware versions when available.':'若有预发布版本则一并显示。',
  'Checks for firmware updates automatically shortly after WiFi connects.':'WiFi 连接后自动检查固件更新。',
  'No update URL':'未提供更新地址',
  'Install firmware update? The device will reboot.':'安装固件更新？设备将会重启。',
  'Install update':'安装更新','Install':'安装',
  'Downloading & installing...':'下载并安装中...',
  'Update installed! Rebooting...':'更新已安装，重启中...',
  'Update failed':'更新失败',
  'Manual firmware upload (.bin)':'手动上传固件 (.bin)',
  'Upload a local firmware .bin file directly to the device.':'将本地固件 .bin 文件直接上传到设备。',
  'Done! Device is rebooting...':'完成！设备正在重启...',
  'OTA Username:':'OTA 用户名：','OTA Password:':'OTA 密码：',
  'Flashing...':'刷写中...','OTA Credentials Reset':'OTA 凭据已重置',
  'Up to date':'已是最新','Update available':'发现可用更新',
  // CAN tools card
  'Sniffer, recorder and bus status':'嗅探、记录与总线状态',
  'Live CAN tools for sniffing, recording, controller status and checking the last injected write.':'实时 CAN 工具：嗅探、记录、控制器状态以及最后写入校验。',
  'Shows the latest 30 CAN frames live. You can filter by ID or name, switch between wire IDs and DBC IDs, and pause the view.':'实时显示最近 30 帧 CAN 报文。可按 ID 或名称过滤，可在线束 ID 和 DBC ID 之间切换并暂停。',
  'Filter by ID or name':'按 ID 或名称过滤',
  'Filter by wire/DBC ID or name':'按 线束/DBC ID 或名称过滤',
  'Records live CAN traffic up to the frame limit and lets you download it as a CSV file.':'实时记录 CAN 报文直到达到上限，可导出为 CSV 文件。',
  'Download CSV':'下载 CSV','Mux':'多路','OK':'正常',
  'Bus-Off':'总线关闭','TX Passive':'TX 被动','RX Passive':'RX 被动',
  'TX Warn':'TX 警告','RX Warn':'RX 警告','RX Overflow':'RX 溢出',
  'Shows CAN controller health, error flags and the RX, TX and error counters per mux.':'显示 CAN 控制器状态、错误标志以及每个 mux 的 RX/TX/错误计数。',
  'Compares the last injected frame with the latest bus frame that has the same CAN ID and mux. Helpful to spot overwrites, but not proof that a module accepted the change.':'比较最近一次注入帧与同一 CAN ID/mux 的最新总线帧。有助于发现覆盖，但不能证明模块已接受变更。',
  'No injected frame yet':'尚未注入帧','Sent':'已发送','Bus':'总线',
  'Waiting for next matching bus frame':'等待下一帧匹配的总线报文',
  'Matching frame seen on bus':'已在总线上看到匹配帧',
  'Latest bus frame differs from injected frame':'最新总线帧与注入帧不一致',
  'Driver transmit failed':'驱动发送失败',
  'No matching RX frame seen yet':'尚未看到匹配的 RX 帧',
  'Reset Stats':'重置统计',
  // CAN debug card
  'Enable debug panels: firmware update, debug log and live CAN tools.':'启用调试面板：固件更新、调试日志和实时 CAN 工具。',
  // Confirm modal / common
  'Confirm':'确认','Continue':'继续','Cancel':'取消',
  // Warnings
  'CAN bus writes affect vehicle behavior. Remove device immediately if unexpected behavior occurs. Not affiliated with any vehicle manufacturer.':'CAN 总线写入会影响车辆行为。一旦出现异常请立即拔除设备。与任何车厂无关联。',
  'Discard current - never lowers.':'????????? UI_mppSpeedLimit (CAN 760 byte 6) ???????? km/h???????????????????????? HW3 ???',
  'Manual firmware upload only. Select a local .bin and flash it to the device.':'?????????????? .bin ???????',
  'Manual OTA':'?? OTA',
  'Use the generated PlatformIO firmware.bin for this board.':'?????????? PlatformIO firmware.bin?',
  'Current build path:':'???????',
  'Set the ESP32 GPIO pins used for the CAN transceiver. Wrong values can disable CAN.':'?? CAN ?????? ESP32 GPIO ?????????? CAN ?????',
  'Custom CAN pins saved in NVS. Reboot required after change.':'??? CAN ?????? NVS?????????',
  'Using firmware default CAN pins. Save only if your hardware wiring differs.':'???????? CAN ??????????????????',
  'Save CAN pins':'?? CAN ??',
  'Restored. Reboot required.':'?????????',

});

// Final Chinese overrides for recently changed WebUI labels.
Object.assign(I18N_ZH,{
  'High-speed boost (>=80 km/h)':'\u9ad8\u901f\u5206\u6bb5\u63d0\u901f\uff08>=80 km/h\uff09',
  '80/100/120 km/h buckets. Max +50% target: 120/150/180 km/h.':'80/100/120 km/h \u5206\u6bb5\u3002\u6700\u5927\u63d0\u901f 50%\uff0c\u76ee\u6807\u4e0a\u9650\uff1a120/150/180 km/h\u3002',
  '30/40/50/60/70 km/h buckets. Max +50% target: 45/60/75/90/105 km/h.':'30/40/50/60/70 km/h \u5206\u6bb5\u3002\u6700\u5927\u63d0\u901f 50%\uff0c\u76ee\u6807\u4e0a\u9650\uff1a45/60/75/90/105 km/h\u3002',
  'Default 25%/s, max 25%/s':'\u9ed8\u8ba4 25%/\u79d2\uff0c\u6700\u9ad8 25%/\u79d2',
  'Use 1-25, max 25':'\u8bf7\u8f93\u5165 1-25\uff0c\u6700\u9ad8 25',
  'Manual firmware upload only. Select a local .bin and flash it to the device.':'\u4ec5\u4fdd\u7559\u624b\u52a8\u56fa\u4ef6\u4e0a\u4f20\u3002\u9009\u62e9\u672c\u5730 .bin \u5e76\u5237\u5199\u5230\u8bbe\u5907\u3002',
  'Manual OTA':'\u624b\u52a8 OTA',
  'Use the generated PlatformIO firmware.bin for this board.':'\u8bf7\u4f7f\u7528\u4e3a\u6b64\u677f\u5361\u751f\u6210\u7684 PlatformIO firmware.bin\u3002',
  'Current build path:':'\u5f53\u524d\u6784\u5efa\u8def\u5f84\uff1a',
  'Set the ESP32 GPIO pins used for the CAN transceiver. Wrong values can disable CAN.':'\u8bbe\u7f6e CAN \u6536\u53d1\u5668\u4f7f\u7528\u7684 ESP32 GPIO \u5f15\u811a\u3002\u9519\u8bef\u914d\u7f6e\u4f1a\u5bfc\u81f4 CAN \u65e0\u6cd5\u5de5\u4f5c\u3002',
  'Custom CAN pins saved in NVS. Reboot required after change.':'\u81ea\u5b9a\u4e49 CAN \u5f15\u811a\u5df2\u4fdd\u5b58\u5728 NVS\uff0c\u4fee\u6539\u540e\u9700\u8981\u91cd\u542f\u3002',
  'Using firmware default CAN pins. Save only if your hardware wiring differs.':'\u6b63\u5728\u4f7f\u7528\u56fa\u4ef6\u9ed8\u8ba4 CAN \u5f15\u811a\u3002\u53ea\u6709\u786c\u4ef6\u63a5\u7ebf\u4e0d\u540c\u65f6\u624d\u9700\u8981\u4fdd\u5b58\u3002',
  'Save CAN pins':'\u4fdd\u5b58 CAN \u5f15\u811a',
  'Restored. Reboot required.':'\u5df2\u8fd8\u539f\uff0c\u9700\u8981\u91cd\u542f\u3002',
  'Raise UI_mppSpeedLimit on CAN 760 byte 6 to a target km/h based on what the gateway is currently sending. Same bucket layout as HW3. Only writes when target is higher than current - never lowers.':'\u6839\u636e\u7f51\u5173\u5f53\u524d\u53d1\u9001\u7684 UI_mppSpeedLimit (CAN 760 byte 6) \u6309\u5206\u6bb5\u8868\u5f97\u5230\u76ee\u6807 km/h\uff0c\u4ec5\u5728\u76ee\u6807\u503c\u9ad8\u4e8e\u5f53\u524d\u503c\u65f6\u5199\u56de\uff0c\u4ece\u4e0d\u964d\u4f4e\u3002\u5206\u6bb5\u5e03\u5c40\u4e0e HW3 \u4e00\u81f4\u3002',
  '80/100/120 km/h buckets. Max target: 120/150/155 km/h.':'80/100/120 km/h \u5206\u6bb5\u3002\u76ee\u6807\u4e0a\u9650\uff1a120/150/155 km/h\u3002',
  'Profiles are available on Legacy, HW3 and HW4.':'Legacy\u3001HW3 \u548c HW4 \u652f\u6301\u914d\u7f6e\u6863\u3002',
  'OTA Test v2':'OTA \u6d4b\u8bd5 v2',
  'Version: 3.0.0-beta.5\nOTA timestamp: 2026-05-23 21:47:24 +08:00':'\u7248\u672c\uff1a3.0.0-beta.5\nOTA \u65f6\u95f4\uff1a2026-05-23 21:47:24 +08:00',
  'AP':'AP',
  'STA':'STA',
  'DNS':'DNS',
  'Upstream':'\u4e0a\u6e38',
  'Clients':'\u5ba2\u6237\u7aef',
  'compiled':'\u5df2\u7f16\u8bd1',
  'not compiled':'\u672a\u7f16\u8bd1',
  'no task':'\u65e0\u4efb\u52a1',
  'bind ok':'\u7ed1\u5b9a\u6b63\u5e38',
  'bind wait':'\u7b49\u5f85\u7ed1\u5b9a',
  'fd':'fd',
  'none':'\u65e0',
  'whitelist override blacklist':'\u767d\u540d\u5355\u8986\u76d6\u9ed1\u540d\u5355'
});
Object.assign(I18N_ZH,{
  'Upstream DNS':'\u4e0a\u6e38 DNS',
  'Auto':'\u81ea\u52a8',
  'Custom':'\u81ea\u5b9a\u4e49',
  'Custom DNS, e.g. 8.8.8.8':'\u81ea\u5b9a\u4e49 DNS\uff0c\u4f8b\u5982 8.8.8.8',
  'Reset DNS Stats':'\u6e05\u96f6 DNS \u7edf\u8ba1',
  'Resetting DNS stats...':'\u6b63\u5728\u6e05\u96f6 DNS \u7edf\u8ba1...',
  'DNS stats reset':'DNS \u7edf\u8ba1\u5df2\u6e05\u96f6',
  'Network Performance Mode':'\u7f51\u7edc\u6027\u80fd\u6a21\u5f0f',
  'Reduce WebUI polling while AP+STA+NAPT is forwarding traffic':'AP+STA+NAPT \u8f6c\u53d1\u6d41\u91cf\u65f6\u964d\u4f4e WebUI \u8f6e\u8be2',
  'ON: status 5s, network diagnostics 30s, heavy lists manual only':'\u5f00\uff1a\u72b6\u6001 5 \u79d2\uff0c\u7f51\u7edc\u8bca\u65ad 30 \u79d2\uff0c\u91cd\u5217\u8868\u4ec5\u624b\u52a8',
  'OFF: status 2s, network diagnostics 10s, DNS/filter lists auto refresh':'\u5173\uff1a\u72b6\u6001 2 \u79d2\uff0c\u7f51\u7edc\u8bca\u65ad 10 \u79d2\uff0cDNS/\u8fc7\u6ee4\u5217\u8868\u81ea\u52a8\u5237\u65b0',
  'invalid upstream DNS':'\u4e0a\u6e38 DNS \u5730\u5740\u65e0\u6548',
  'custom upstream DNS required':'\u9700\u8981\u586b\u5199\u81ea\u5b9a\u4e49\u4e0a\u6e38 DNS',
  'Auto uses DHCP DNS from the connected WiFi; public DNS can avoid stale slow/fail counters from a bad router DNS.':'\u81ea\u52a8\u4f7f\u7528\u5df2\u8fde\u63a5 WiFi \u5206\u914d\u7684 DHCP DNS\uff1b\u516c\u5171 DNS \u53ef\u4ee5\u907f\u514d\u8def\u7531\u5668 DNS \u5f02\u5e38\u5bfc\u81f4\u7684 slow/fail \u7d2f\u8ba1\u8bef\u5224\u3002',
  'Using Ali DNS 223.5.5.5.':'\u4f7f\u7528\u963f\u91cc DNS 223.5.5.5\u3002',
  'Using Tencent DNS 119.29.29.29.':'\u4f7f\u7528\u817e\u8baf DNS 119.29.29.29\u3002',
  'Enter a custom upstream DNS IPv4 address.':'\u8f93\u5165\u81ea\u5b9a\u4e49\u4e0a\u6e38 DNS IPv4 \u5730\u5740\u3002'
});
const I18N_EN={};Object.keys(I18N_ZH).forEach(k=>I18N_EN[I18N_ZH[k]]=k);
Object.assign(I18N_EN,{
  'CAN 调试':'CAN Debug',
  'CAN调试':'CAN Debug'
});
const I18N_RX=[
  [/^Enabled \u2022 NAT on \u2022 blocked (\d+)$/,'已启用 \u2022 NAT 开启 \u2022 已阻断 $1'],
  [/^Enabled \u2022 NAT waiting \u2022 blocked (\d+)$/,'已启用 \u2022 NAT 等待中 \u2022 已阻断 $1'],
  [/^Disabled \u2022 NAT waiting \u2022 blocked (\d+)$/,'已禁用 \u2022 NAT 等待中 \u2022 已阻断 $1'],
  [/^Enabled \u2022 NAT on \u2022 blocked (\d+) \u2022 DNS cache (\d+)\/(\d+)$/,'已启用 \u2022 NAT 开启 \u2022 已阻断 $1 \u2022 DNS 缓存 $2/$3'],
  [/^Enabled \u2022 NAT waiting \u2022 blocked (\d+) \u2022 DNS cache (\d+)\/(\d+)$/,'已启用 \u2022 NAT 等待中 \u2022 已阻断 $1 \u2022 DNS 缓存 $2/$3'],
  [/^Disabled \u2022 NAT waiting \u2022 blocked (\d+) \u2022 DNS cache (\d+)\/(\d+)$/,'已禁用 \u2022 NAT 等待中 \u2022 已阻断 $1 \u2022 DNS 缓存 $2/$3'],
  [/^Connected: (.+) \u2022 ([0-9a-fA-F:.]+) \u2022 switch to that WiFi and open this IP$/,'已连接：$1 \u2022 $2 \u2022 请切换到该 WiFi 并打开此 IP'],
  [/^Connected: (.+) \u2022 ([0-9a-fA-F:.]+)$/,'已连接：$1 \u2022 $2'],
  [/^Add network \((\d+)\/(\d+)\)$/,'添加网络 ($1/$2)'],
  [/^Connected: (.+)$/,'已连接：$1'],[/^Connecting to (.+)\.\.\.$/,'正在连接 $1...'],
  [/^(\d+) saved \u2022 trying to connect\.\.\.$/,'已保存 $1 个 \u2022 正在尝试连接...'],
  [/^(\d+) client(s?)$/,'$1 个客户端'],[/^(\d+) frames$/,'$1 帧'],[/^(\d+) \/ (\d+) frames$/,'$1 / $2 帧'],
  [/^(\d+) frames saved$/,'已保存 $1 帧'],
  [/^Up to date \(v(.+)\)$/,'已是最新 (v$1)'],[/^Update available!$/,'发现可用更新！'],
  [/^(.+)\s*[\u2022?]\s*(\d+) cores\s*[\u2022?]\s*(\d+) MHz now$/,'$1 \u2022 $2 核 \u2022 当前 $3 MHz'],
  [/^(\d+) cores\s*[\u2022?]\s*now (\d+) MHz\s*[\u2022?]\s*max (\d+) MHz$/,'$1 核 \u2022 当前 $2 MHz \u2022 最大 $3 MHz'],
  [/^CPU0 (\d+)%\s*[\u2022?]\s*CPU1 (\d+)%$/,'CPU0 $1% \u2022 CPU1 $2%'],
  [/^(\d+) tasks$/,'$1 个任务'],
  [/^(\d+) installed$/,'已安装 $1 个'],
  [/^(\d+) \/ (\d+) installed$/,'已安装 $1 / $2 个'],
  [/^(\d+) rule$/,'$1 条规则'],[/^(\d+) rules$/,'$1 条规则'],
  [/^Max (\d+) networks$/,'最多 $1 个网络'],
  [/^Delete network "(.+)"\?$/,'确认删除网络 "$1"？'],
  [/^Save CAN pins TX=(\d+) RX=(\d+) and reboot\? Wrong pins disable CAN\.$/,'保存 CAN 引脚 TX=$1 RX=$2 并重启？错误引脚会导致 CAN 无法工作。'],
  [/^custom TX=(\d+) RX=(\d+)$/,'自定义 TX=$1 RX=$2'],
  [/^firmware default TX=(\d+) RX=(\d+)$/,'固件默认 TX=$1 RX=$2'],
  [/^Restore settings from (.+) and reboot\?$/,'从 $1 还原设置并重启？'],
  [/^Loaded "(.+)" into editor$/,'已将 "$1" 加载到编辑器'],
  [/^Use 1-(\d+)$/,'请输入 1-$1'],
  [/^Done (\d+)\/(\d+)$/,'已完成 $1/$2'],
  [/^Stopped (\d+)\/(\d+)$/,'已停止 $1/$2'],
  [/^Running (\d+)\/(\d+) · every (\d+) ms$/,'运行中 $1/$2 · 间隔 $3 毫秒'],
  [/^Waiting for CAN 0x([0-9A-Fa-f]+)$/,'等待 CAN 0x$1'],
  [/^Whitelist (\d+)\/(\d+)\s*[\u2022?]\s*Blacklist (\d+)\/(\d+)$/,'白名单 $1/$2 \u2022 黑名单 $3/$4'],
  [/^allowed (\d+)\s*[\u2022?]\s*blocked (\d+)$/,'允许 $1 \u2022 阻断 $2'],
  [/^Connection to (.+) lost\. Reload after reconnecting\.$/,'与 $1 的连接已断开，重新连接后请刷新。'],
  [/^Connection to (.+) lost\. Switch to your normal WiFi and open http:\/\/(.+)$/,'与 $1 的连接已断开，请切换到常用 WiFi 后打开 http://$2'],
  [/^GTW_autopilot: ([A-Z_]+) \((\d+)\)$/,'GTW_autopilot：$1 ($2)'],
  [/^(\d+)%\/s \(about ([\d.]+) km\/h\/s at 60 km\/h\)$/,'$1%/秒（60 km/h 时约 $2 km/h/秒）'],
  [/^(\d+)%\/s$/,'$1%/秒'],
  [/^Showing DBC JSON IDs with (.+) prefix$/,'显示带 $1 前缀的 DBC JSON ID']
];
function trText(value){
  let s=String(value);
  if(dashLang!=='zh')return I18N_EN[s]||s;
  if(I18N_ZH[s])return I18N_ZH[s];
  for(const r of I18N_RX){if(r[0].test(s))return s.replace(r[0],r[1]);}
  return s;
}
const setText=(id,value)=>{const el=$(id);if(el)el.textContent=trText(value);};
const setClass=(id,value)=>{const el=$(id);if(el)el.className=value;};
function profileNamesForHw(hw){return hw===2?SP4:SP3;}
function profileDisplayName(hw,sp,auto){
  const name=(profileNamesForHw(hw)||[])[clampProfileForHw(hw,sp)]||'--';
  return auto?'Auto ('+name+')':name;
}
function gtwAutopilotName(v){
  return ['NONE','HIGHWAY','ENHANCED','SELF_DRIVING','BASIC'][v]||'UNKNOWN';
}
function gtwAutopilotShort(v,ad){
  v=Number(v);
  if(v===3)return 'AP-FSD';
  if(v===2)return 'AP-EAP';
  if(v===4)return 'AP-BASIC';
  if(v===1)return 'AP-HWY';
  if(v===0)return 'AP-NONE';
  return ad?'AD':'AP-?';
}
function gtwAutopilotBadge(v){
  if(v<0)return 'GTW --';
  if(v===3)return 'GTW SELF';
  return 'GTW '+gtwAutopilotName(v);
}
function injectionStatusLabel(injecting,armed,apGate,d){
  if(injecting){
    const tag=gtwAutopilotShort(d.gtwap,d.apActive);
    return (dashLang==='zh'?'运行中':'Active')+' '+tag;
  }
  if(armed&&apGate)return dashLang==='zh'?'等待 AP':'Waiting AP';
  return dashLang==='zh'?'已阻止':'BLOCKED';
}
function updateGtwBadge(v){
  const el=$('gtw-badge');if(!el)return;
  v=Number(v);
  const known=!isNaN(v)&&v>=0;
  el.textContent=gtwAutopilotBadge(known?v:-1);
  el.className='gtw-badge '+(known?'known':'');
  el.title=known?trText('GTW_autopilot: '+gtwAutopilotName(v)+' ('+v+')'):trText('GTW_autopilot: not seen yet');
}
let state={hw:1,can:true,sp:0,spAuto:true,hw3OffsetSlew:false,hw3SlewRate:25};
let sniffPaused=false,sniffFrames=[];
let sniffShowDbcIds=localStorage.getItem('sniffIdMode')==='dbc';
let otaFile=null;
let otaUser=localStorage.getItem('otaU')||'',otaPass=localStorage.getItem('otaP')||'';
let logSince=0;
let dashConfirmState=null;
let supportIssueUrl='https://github.com/ev-open-can-tools/ev-open-can-tools/issues/new?template=issue.yml';
let supportBodyText='';
let dashboardPollTimers=[];
let dashboardPollFailures=0;
let dashboardStatusOk=false;
let dashboardInitialLoaded=false;
let dashboardPollStopped=false;
let systemStatusTimer=null;
let systemStatusEnabled=false;
let taskStatsTimer=null;
let dashboardStaIp='';
let canDebugEnabled=localStorage.getItem('canDebug')==='1';
let canDebugPollTimers=[];
let networkPerformanceMode=localStorage.getItem('netPerfMode')!=='0';
const pollLocks={};

function stopDashboardPolling(){
  if(dashboardPollStopped)return;
  dashboardPollStopped=true;
  dashboardPollTimers.forEach(clearInterval);
  dashboardPollTimers=[];
  if(systemStatusTimer){clearInterval(systemStatusTimer);systemStatusTimer=null;}
  if(taskStatsTimer){clearInterval(taskStatsTimer);taskStatsTimer=null;}
  stopCanDebugPolling();
  $('dot').className='sdot dot-off';
  $('hdr-desc').textContent='Dashboard disconnected';
  let msg='Connection to '+location.hostname+' lost. Reload after reconnecting.';
  if(dashboardStaIp&&dashboardStaIp!==location.hostname)msg='Connection to '+location.hostname+' lost. Switch to your normal WiFi and open http://'+dashboardStaIp;
  $('wifi-status').textContent=msg;
  $('wifi-status').style.color='var(--err)';
}

function dashboardVisible(){
  return !document.hidden&&!dashboardPollStopped;
}
function intervalVisible(fn,ms){
  return setInterval(()=>{if(dashboardVisible())fn();},ms);
}

function updateNetworkPerformanceUi(){
  const t=$('net-perf-tgl');if(t)t.checked=networkPerformanceMode;
  const s=$('net-perf-status');
  if(s){
    s.textContent=networkPerformanceMode
      ?'ON: status 5s, network diagnostics 30s, heavy lists manual only'
      :'OFF: status 2s, network diagnostics 10s, DNS/filter lists auto refresh';
    s.style.color=networkPerformanceMode?'var(--ok)':'var(--warn)';
    applyDashboardI18n(s);
  }
}
function clearDashboardPollingIntervals(){
  dashboardPollTimers.forEach(clearInterval);
  dashboardPollTimers=[];
}
async function loadBus2Ids(){
  return runPoll('bus2_ids',async()=>{
    if(!dashboardStatusOk)return;
    try{const d=await fetchPollJson('/bus2_ids',2000);
    setText('bus2-count',d.count||0);
    const tb=$('bus2-rows');if(!tb)return;
    if(!d.ids||!d.ids.length){tb.innerHTML='<tr><td colspan="4" class="v-dim">waiting…</td></tr>';return;}
    d.ids.sort((a,b)=>parseInt(a.id,16)-parseInt(b.id,16));
    let h='';
    for(const it of d.ids){h+='<tr><td>0x'+it.id+'</td><td>'+it.dlc+'</td><td>'+it.count+'</td><td style="font-family:monospace">'+(it.data||'')+'</td></tr>';}
    tb.innerHTML=h;
    }catch(e){}
  });
}
function startDashboardPolling(){
  clearDashboardPollingIntervals();
  const fast=!networkPerformanceMode;
  dashboardPollTimers.push(intervalVisible(poll,fast?2000:5000));
  dashboardPollTimers.push(intervalVisible(loadBus2Ids,fast?3000:8000));
  dashboardPollTimers.push(intervalVisible(loadWifiStatus,fast?10000:30000));
  dashboardPollTimers.push(intervalVisible(loadApStatus,fast?10000:30000));
  dashboardPollTimers.push(intervalVisible(loadGatewayStatus,fast?10000:30000));
  if(fast){
    dashboardPollTimers.push(intervalVisible(loadWifiNetworks,30000));
    dashboardPollTimers.push(intervalVisible(loadGatewayBlocked,5000));
    dashboardPollTimers.push(intervalVisible(()=>loadGatewayDns(true),15000));
  }
  updateNetworkPerformanceUi();
}
function setNetworkPerformanceMode(enabled,persist){
  networkPerformanceMode=!!enabled;
  if(persist)localStorage.setItem('netPerfMode',networkPerformanceMode?'1':'0');
  startDashboardPolling();
  if(dashboardVisible()){
    poll();loadWifiStatus();loadApStatus();loadGatewayStatus();
    if(!networkPerformanceMode){loadWifiNetworks();loadGatewayBlocked();loadGatewayDns(true);}
  }
}

function setCanDebugUi(){
  document.body.classList.toggle('can-debug-on',canDebugEnabled);
  const t=$('can-debug-tgl');if(t)t.checked=canDebugEnabled;
  setText('can-debug-meta',canDebugEnabled?'On':'Off');
}
function positionCanDebugPanels(){
  const anchor=$('can-debug-card');if(!anchor)return;
  let after=anchor;
  Array.from(document.querySelectorAll('body > .can-debug-panel')).forEach(panel=>{
    if(panel!==after.nextSibling)after.parentNode.insertBefore(panel,after.nextSibling);
    after=panel;
  });
}
function startCanDebugPolling(){
  if(canDebugPollTimers.length||dashboardPollStopped)return;
  canDebugPollTimers.push(setInterval(pollLog,5000));
  canDebugPollTimers.push(setInterval(pollSniffer,1000));
  pollLog();pollSniffer();pollRec();if(typeof loadUpdateInfo==='function')loadUpdateInfo();if(typeof peRender==='function')peRender();
}
function stopCanDebugPolling(){
  canDebugPollTimers.forEach(clearInterval);
  canDebugPollTimers=[];
  if(recIsActive){stopRec();}
}
function applyCanDebug(){
  setCanDebugUi();
  if(canDebugEnabled)startCanDebugPolling();
  else stopCanDebugPolling();
}
function toggleCanDebug(){
  canDebugEnabled=!!$('can-debug-tgl').checked;
  localStorage.setItem('canDebug',canDebugEnabled?'1':'0');
  applyCanDebug();
}

function noteDashboardPoll(ok){
  if(ok){dashboardPollFailures=0;dashboardStatusOk=true;return;}
  if(dashboardPollStopped)return;
  dashboardStatusOk=false;
  dashboardPollFailures++;
  $('dot').className='sdot dot-off';
  $('hdr-desc').textContent='Dashboard reconnecting';
}

async function fetchPollJson(url,timeoutMs,trackConnection){
  const ctrl=new AbortController();
  const timer=setTimeout(()=>ctrl.abort(),timeoutMs||2500);
  try{
    const r=await fetch(url,{signal:ctrl.signal});
    if(!r.ok)throw new Error('HTTP '+r.status);
    const d=await r.json();
    if(trackConnection)noteDashboardPoll(true);
    return d;
  }catch(e){
    if(trackConnection)noteDashboardPoll(false);
    throw e;
  }finally{
    clearTimeout(timer);
  }
}

async function runPoll(name,fn){
  if(document.hidden)return;
  if(dashboardPollStopped||pollLocks[name])return;
  pollLocks[name]=true;
  try{return await fn();}finally{pollLocks[name]=false;}
}

function waitMs(ms){return new Promise(resolve=>setTimeout(resolve,ms));}

function orderDashboardCards(){
  const stat=document.querySelector('.stat-grid');
  if(!stat||!stat.parentNode)return;
  const cards=Array.from(document.querySelectorAll('.card'));
  const findCard=label=>cards.find(c=>{const t=c.querySelector('.card-title');return t&&t.textContent.trim().toLowerCase().startsWith(label);});
  [findCard('configuration'),findCard('system status'),$('can-debug-card')].filter(Boolean).reverse().forEach(c=>{
    stat.parentNode.insertBefore(c,stat.nextSibling);
  });
}
function initCardMinimizers(){
  document.querySelectorAll('.card').forEach((card,i)=>{
    const hdr=card.querySelector('.card-hdr');if(!hdr||hdr.querySelector('.card-min-btn'))return;
    const title=card.querySelector('.card-title');
    const key='cardCollapse:v2:'+i+':'+((title?title.textContent:'card').trim().toLowerCase().replace(/[^a-z0-9]+/g,'-'));
    card.dataset.collapseKey=key;
    const btn=document.createElement('button');
    btn.type='button';
    btn.className='sniff-btn card-min-btn';
    btn.onclick=()=>{
      const collapsed=!card.classList.contains('collapsed');
      card.classList.toggle('collapsed',collapsed);
      localStorage.setItem(key,collapsed?'1':'0');
      btn.textContent=collapsed?'Show':'Hide';
    };
    hdr.appendChild(btn);
    const stored=localStorage.getItem(key);
    const collapsed=stored===null?true:stored==='1';
    card.classList.toggle('collapsed',collapsed);
    btn.textContent=collapsed?'Show':'Hide';
  });
}
function initSubsectionMinimizers(){
  document.querySelectorAll('.subsec').forEach((sec,i)=>{
    const hdr=sec.querySelector('.subsec-head');if(!hdr||hdr.querySelector('.subsec-btn'))return;
    const explicitKey=sec.dataset.subkey||'';
    const title=sec.querySelector('.subsec-title');
    const safe=((title?title.textContent:'section').trim().toLowerCase().replace(/[^a-z0-9]+/g,'-'));
    const key='subCollapse:v2:'+(explicitKey||i+':'+safe);
    sec.dataset.collapseKey=key;
    const btn=document.createElement('button');
    btn.type='button';
    btn.className='sniff-btn subsec-btn';
    btn.onclick=()=>{
      const collapsed=!sec.classList.contains('collapsed');
      sec.classList.toggle('collapsed',collapsed);
      localStorage.setItem(key,collapsed?'1':'0');
      btn.textContent=collapsed?'Show':'Hide';
    };
    hdr.appendChild(btn);
    const stored=localStorage.getItem(key);
    const collapsed=stored===null?true:stored==='1';
    sec.classList.toggle('collapsed',collapsed);
    btn.textContent=collapsed?'Show':'Hide';
  });
}

function syncSniffPauseButton(){
  const b=$('sniff-pause-btn');if(!b)return;
  b.textContent=sniffPaused?'Resume':'Pause';
  b.classList.toggle('paused',sniffPaused);
}

function actionErrorMessage(e,fallback){
  if(!e)return fallback;
  if(e.name==='AbortError'||e.name==='SyntaxError'||e.message==='Failed to fetch'||e.message==='Empty response')return fallback;
  return e.message||fallback;
}

async function fetchJsonWithTimeout(url,options,timeoutMs){
  const ctrl=new AbortController();
  const timer=setTimeout(()=>ctrl.abort(),timeoutMs||2500);
  try{
    const opts=Object.assign({},options||{});
    opts.signal=ctrl.signal;
    const r=await fetch(url,opts);
    const text=await r.text();
    if(!text||!text.trim())throw new Error(r.ok?'Empty response':('HTTP '+r.status));
    const d=JSON.parse(text);
    if(!r.ok)throw new Error(d.error||('HTTP '+r.status));
    return d;
  }finally{
    clearTimeout(timer);
  }
}

function showOwnerNotice(){
  const modal=$('owner-modal');
  if(!modal)return;
  modal.style.display='flex';
  document.body.style.overflow='hidden';
}
function closeOwnerNotice(){
  const modal=$('owner-modal');
  if(modal)modal.style.display='none';
  document.body.style.overflow='';
}
function ownerNoticeBackdrop(ev){
  if(ev.target===$('owner-modal'))closeOwnerNotice();
}
function showOtaTestNotice(){
  closeOwnerNotice();
  const modal=$('ota-test-modal');
  if(!modal)return;
  modal.style.display='flex';
  document.body.style.overflow='hidden';
}
function closeOtaTestNotice(){
  const modal=$('ota-test-modal');
  if(modal)modal.style.display='none';
  document.body.style.overflow='';
}
function otaTestBackdrop(ev){
  if(ev.target===$('ota-test-modal'))closeOtaTestNotice();
}

function showOtaTestNotice(){
  closeOwnerNotice();
  const modal=$('ota-test-modal');
  if(!modal)return;
  modal.style.display='flex';
  document.body.style.overflow='hidden';
}
function closeOtaTestNotice(){
  const modal=$('ota-test-modal');
  if(modal)modal.style.display='none';
  document.body.style.overflow='';
}
function otaTestBackdrop(ev){
  if(ev.target===$('ota-test-modal'))closeOtaTestNotice();
}

function dashConfirmResolve(ok){
  if(!dashConfirmState)return;
  const resolve=dashConfirmState.resolve;
  dashConfirmState=null;
  $('confirm-modal').style.display='none';
  document.body.style.overflow='';
  resolve(!!ok);
}

function dashConfirmBackdrop(ev){
  if(ev.target===$('confirm-modal'))dashConfirmResolve(false);
}

function supportBackdrop(ev){
  if(ev.target===$('support-modal'))closeSupport();
}

function dashConfirm(message,title,okText,cancelText){
  if(dashConfirmState)dashConfirmResolve(false);
  return new Promise(resolve=>{
    dashConfirmState={resolve};
    $('confirm-title').textContent=title||'Confirm';
    $('confirm-msg').textContent=message||'';
    $('confirm-ok').textContent=okText||'Continue';
    $('confirm-cancel').textContent=cancelText||'Cancel';
    $('confirm-modal').style.display='flex';
    document.body.style.overflow='hidden';
    setTimeout(()=>{$('confirm-ok').focus();},0);
  });
}

function supportSettingsSummary(){
  return [
    'Hardware: '+(HW[state.hw]||'?'),
    'Speed profile: '+profileDisplayName(state.hw,state.sp,state.spAuto),
    'CAN status: '+($('s-can')?$('s-can').textContent:'--'),
    'Injection: '+($('s-inj')?$('s-inj').textContent:'--'),
    'AD: '+($('s-AD')?$('s-AD').textContent:'--'),
    'CAN pins: '+($('can-pins-status')?$('can-pins-status').textContent:'--'),
    'Firmware: '+($('fw-ver')?$('fw-ver').textContent:'--'),
    'Beta channel: '+($('beta-tgl')&&$('beta-tgl').checked?'enabled':'disabled'),
    'Auto-update: '+($('auto-upd-tgl')&&$('auto-upd-tgl').checked?'enabled':'disabled'),
    'HW3 offset slew: '+(state.hw3OffsetSlew?'enabled @ '+(state.hw3SlewRate||25)+'%/s':'disabled'),
    'Dashboard logging: '+($('tgl-eprn')&&$('tgl-eprn').checked?'enabled':'disabled')
  ].join('\n');
}

function buildSupportBody(){
  const body=[
    'ev-open-can-tools support report',
    '',
    'Device',
    'Hardware: '+(HW[state.hw]||'?'),
    'Speed profile: '+profileDisplayName(state.hw,state.sp,state.spAuto),
    'CAN status: '+($('s-can')?$('s-can').textContent:'--'),
    'Injection: '+($('s-inj')?$('s-inj').textContent:'--'),
    'AD: '+($('s-AD')?$('s-AD').textContent:'--'),
    'CAN pins: '+($('can-pins-status')?$('can-pins-status').textContent:'--'),
    'Firmware: '+($('fw-ver')?$('fw-ver').textContent:'--'),
    '',
    'Settings',
    'Beta channel: '+($('beta-tgl')&&$('beta-tgl').checked?'enabled':'disabled'),
    'Auto-update: '+($('auto-upd-tgl')&&$('auto-upd-tgl').checked?'enabled':'disabled'),
    'HW3 offset slew: '+(state.hw3OffsetSlew?'enabled @ '+(state.hw3SlewRate||25)+'%/s':'disabled'),
    'Dashboard logging: '+($('tgl-eprn')&&$('tgl-eprn').checked?'enabled':'disabled'),
    '',
    'Notes',
    ''
  ].join('\n');
  supportBodyText=body;
  return body;
}

function openSupport(){
  const el=$('support-body');
  if(el)el.value=buildSupportBody();
  const st=$('support-status');
  if(st){st.textContent='Copy this text, then open the GitHub issue form.';st.style.color='var(--tx3)';}
  $('support-modal').style.display='flex';
  document.body.style.overflow='hidden';
  setTimeout(()=>{if(el)el.focus();el&&el.setSelectionRange(0,0);},0);
}

function closeSupport(){
  $('support-modal').style.display='none';
  document.body.style.overflow='';
}

function copySupportText(text,el){
  if(el){
    el.focus();
    el.select();
    el.setSelectionRange(0,text.length);
    if(document.execCommand&&document.execCommand('copy'))return true;
  }
  if(navigator.clipboard&&navigator.clipboard.writeText){
    navigator.clipboard.writeText(text).catch(()=>{});
    return true;
  }
  return false;
}

function copySupport(){
  const el=$('support-body');
  const text=el?el.value:buildSupportBody();
  if(copySupportText(text,el)){
    const st=$('support-status');if(st){st.textContent='Copied to clipboard';st.style.color='var(--ok)';}
    return true;
  }
  const st=$('support-status');if(st){st.textContent='Copy failed';st.style.color='var(--err)';}
  return false;
}

function openSupportIssue(){
  const url='https://github.com/ev-open-can-tools/ev-open-can-tools/issues/new?template=issue.yml';
  const copied=copySupport();
  supportIssueUrl=url;
  window.open(url,'_blank','noopener');
  const st=$('support-status');if(st&&copied){st.textContent='Copied support details. Paste them into the support question.';st.style.color='var(--ok)';}
  closeSupport();
}

document.addEventListener('keydown',e=>{
  if(e.key==='Escape'){
    if(dashConfirmState)dashConfirmResolve(false);
    closeHelpPanels(document);
  }
});
document.addEventListener('click',e=>{
  if(!e.target.closest('.title-help')&&!e.target.closest('.inline-help-panel')){
    closeHelpPanels(document);
  }
});

function toggleTheme(){
  const html=document.documentElement;
  const isDark=html.getAttribute('data-theme')==='dark';
  html.setAttribute('data-theme',isDark?'light':'dark');
  $('theme-btn').innerHTML=isDark?'&#9790; '+trText('Dark'):'&#9788; '+trText('Light');
  localStorage.setItem('theme',isDark?'light':'dark');
}
function i18nSkip(el){
  return !el||['SCRIPT','STYLE','TEXTAREA','INPUT','OPTION'].includes(el.nodeName);
}
function i18nNodeText(node){
  if(!node||!node.nodeValue||!node.nodeValue.trim()||i18nSkip(node.parentElement))return;
  const raw=node.nodeValue;
  const lead=(raw.match(/^\s*/)||[''])[0],tail=(raw.match(/\s*$/)||[''])[0];
  const mid=raw.trim();
  const out=trText(mid);
  if(out!==mid)node.nodeValue=lead+out+tail;
}
function i18nElementAttrs(el){
  if(!el||['SCRIPT','STYLE'].includes(el.nodeName))return;
  ['placeholder','title','aria-label'].forEach(a=>{const v=el.getAttribute&&el.getAttribute(a);if(v){const t=trText(v);if(t!==v)el.setAttribute(a,t);}});
}
function applyDashboardI18n(root){
  root=root||document.body;
  if(!root)return;
  if(root.nodeType===Node.TEXT_NODE){i18nNodeText(root);return;}
  i18nElementAttrs(root);
  const walker=document.createTreeWalker(root,NodeFilter.SHOW_TEXT,{acceptNode:n=>i18nSkip(n.parentElement)?NodeFilter.FILTER_REJECT:NodeFilter.FILTER_ACCEPT});
  let n;while((n=walker.nextNode()))i18nNodeText(n);
  root.querySelectorAll&&root.querySelectorAll('[placeholder],[title],[aria-label]').forEach(i18nElementAttrs);
  updateLanguageButton();
}
function updateLanguageButton(){
  const b=$('lang-btn');if(b)b.textContent=dashLang==='zh'?'English':'\u4e2d\u6587';
}
function toggleLanguage(){
  dashLang=dashLang==='zh'?'en':'zh';
  localStorage.setItem('dashLang',dashLang);
  applyDashboardI18n(document.body);
  const t=document.documentElement.getAttribute('data-theme')||'dark';
  $('theme-btn').innerHTML=t==='dark'?'&#9788; '+trText('Light'):'&#9790; '+trText('Dark');
}
(function(){
  const t=localStorage.getItem('theme')||'dark';
  document.documentElement.setAttribute('data-theme',t);
  // will be updated after DOM ready
  window.addEventListener('DOMContentLoaded',()=>{
    $('theme-btn').innerHTML=t==='dark'?'&#9788; '+trText('Light'):'&#9790; '+trText('Dark');
    updateLanguageButton();
    applyDashboardI18n(document.body);
    setTimeout(showOwnerNotice,120);
    const obs=new MutationObserver(muts=>{
      if(dashLang!=='zh')return;
      muts.forEach(m=>{
        m.addedNodes&&m.addedNodes.forEach(n=>applyDashboardI18n(n));
        if(m.type==='characterData')i18nNodeText(m.target);
      });
    });
    obs.observe(document.body,{childList:true,subtree:true,characterData:true});
  });
})();

function updateHW4(hw){
  document.querySelectorAll('.hw4-only').forEach(el=>el.classList.toggle('hidden',hw!==2));
}

function updateHardwareDependentSections(hw){
  const speedSec=$('hw3-speed-section');
  const slewSec=$('hw3-slew-section');
  const legacySec=$('legacy-mpp-section');
  if(speedSec)speedSec.style.display=hw===1?'':'none';
  if(slewSec)slewSec.style.display=hw===1?'':'none';
  if(legacySec)legacySec.style.display=hw===0?'':'none';
}

function expandActiveHardwareSection(hw){
  const sec=hw===0?$('legacy-mpp-section'):hw===1?$('hw3-speed-section'):null;
  if(!sec)return;
  sec.classList.remove('collapsed');
  if(sec.dataset.collapseKey)localStorage.setItem(sec.dataset.collapseKey,'0');
  const btn=sec.querySelector('.subsec-btn');
  if(btn)btn.textContent='Hide';
}

function clampProfileForHw(hw,sp){
  if(hw===2)return Math.max(0,Math.min(4,Number(sp)||0));
  if(hw===0||hw===1)return Math.max(0,Math.min(2,Number(sp)||0));
  return 0;
}

function updateProfileControls(hw,sp,spAuto){
  const sp3=$('sp3-group'),sp4=$('sp4-group'),note=$('profile-note');
  const safeSp=clampProfileForHw(hw,sp);
  if(sp3)sp3.classList.toggle('hidden',!(hw===0||hw===1));
  if(sp4)sp4.classList.toggle('hidden',hw!==2);
  updateHardwareDependentSections(hw);
  const sp3Seg=$('sp3-seg'),sp4Seg=$('sp4-seg');
  updateProfileSeg(sp3Seg,safeSp,spAuto);
  updateProfileSeg(sp4Seg,safeSp,spAuto);
  if(note){
    if(spAuto)note.textContent='Auto follows the vehicle follow distance.';
    else if(hw===0||hw===1)note.textContent='Manual SP3 profile is locked.';
    else if(hw===2)note.textContent='Manual SP4 profile is locked.';
    else note.textContent='Profiles are available on Legacy, HW3 and HW4.';
  }
}

function updateProfileSeg(el,sp,spAuto){
  if(!el)return;
  el.querySelectorAll('.hw-btn').forEach(b=>{
    const v=parseInt(b.dataset.v);
    b.classList.toggle('active',spAuto?v===-1:v===sp);
  });
}

function updSeg(el,v,cls){
  el.querySelectorAll('.'+cls).forEach(b=>b.classList.toggle('active',parseInt(b.dataset.v)===v));
}

function setHW(v){state.hw=v;state.sp=clampProfileForHw(v,state.sp);updSeg($('hw-seg'),v,'hw-btn');updateHW4(v);updateProfileControls(v,state.sp,state.spAuto);expandActiveHardwareSection(v);updateSniffIdToggle();renderSniffer();pushCfg();}

function setProfileAuto(){
  state.spAuto=true;
  updateProfileControls(state.hw,state.sp,state.spAuto);
  pushCfg();
}

function setProfile(v){
  state.spAuto=false;
  state.sp=clampProfileForHw(state.hw,v);
  updateProfileControls(state.hw,state.sp,state.spAuto);
  pushCfg();
}

function updateInjectButtons(active){
  const btn=$('btn-fsd-toggle');
  if(btn){
    btn.textContent=trText(active?'Turn FSD Off':'Turn FSD On');
    btn.classList.toggle('btn-stop',!!active);
    if(!active){
      btn.style.background='var(--accBg)';
      btn.style.color='var(--acc)';
      btn.style.borderColor='var(--accBd)';
    }else{
      btn.style.background='';
      btn.style.color='';
      btn.style.borderColor='';
    }
  }
}

function updateFsdControl(d){
  const enabled=!!d.ci;
  state.can=enabled;
  const tgl=$('fsd-tgl');if(tgl)tgl.checked=enabled;
  const apRestore=$('ap-restore-tgl');if(apRestore&&typeof d.apAutoRestore!=='undefined')apRestore.checked=!!d.apAutoRestore;
  setText('fsd-meta',enabled?'On':'Off');
  const st=$('fsd-status');
  if(st){
    st.textContent=enabled?
      'Built-in FSD chain is active. Legacy/HW3/HW4 injection is controlled by this switch.':
      'FSD chain and CAN injection are disabled and stay off after reboot.';
    st.style.color=enabled?'var(--ok)':'var(--tx3)';
  }
}
async function saveFsdSwitch(){
  const tgl=$('fsd-tgl'),st=$('fsd-status');
  const enabled=tgl&&tgl.checked?'1':'0';
  if(st){st.textContent='Saving...';st.style.color='var(--tx3)';}
  try{
    const r=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'can='+enabled});
    const d=await r.json();
    if(!d.ok)throw new Error();
    state.can=enabled==='1';
    if(st){st.textContent=state.can?'Built-in FSD chain is active.':'FSD chain and CAN injection are disabled.';st.style.color=state.can?'var(--ok)':'var(--tx3)';}
    poll();
  }catch(e){if(st){st.textContent='Save failed';st.style.color='var(--err)';}}
}

async function saveApRestore(){
  const t=$('ap-restore-tgl');
  if(!t)return;
  try{
    const r=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'apRestore='+(t.checked?'1':'0')});
    if(!r.ok)throw new Error('HTTP '+r.status);
  }catch(e){
    addLog('AP/EAP auto restore save failed','le');
  }
}

function sniffBusPrefix(){return state.hw===0?0x0800:0x1000;}
function sniffBusLabel(){return state.hw===0?'PARTY':'CH';}
function sniffWireId(id){return id&0x7FF;}
function sniffDbcId(id){return sniffWireId(id)|sniffBusPrefix();}
function sniffDisplayId(id){return sniffShowDbcIds?sniffDbcId(id):sniffWireId(id);}
function updateSniffIdToggle(){
  const b=$('sniff-id-btn'),bus=sniffBusLabel();
  b.textContent=sniffShowDbcIds?('DBC '+bus):'Wire IDs';
  b.title=sniffShowDbcIds?trText('Showing DBC JSON IDs with '+bus+' prefix'):trText('Showing on-wire 11-bit CAN IDs');
  $('sniff-filter').placeholder=trText('Filter by wire/DBC ID or name');
}
function toggleSniffIdMode(){
  sniffShowDbcIds=!sniffShowDbcIds;
  localStorage.setItem('sniffIdMode',sniffShowDbcIds?'dbc':'wire');
  updateSniffIdToggle();
  renderSniffer();
}

async function pushCfg(){
  const body='hw='+state.hw+'&sp='+state.sp+'&spa='+(state.spAuto?'1':'0')+'&can='+(state.can?'1':'0');
  try{await fetch('/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});}catch(e){}
}

function updateHw3SlewControl(d){
  const enabled=!!d.hw3OffsetSlew;
  const rate=Math.max(1,Math.min(25,parseInt(d.hw3SlewRate,10)||25));
  state.hw3OffsetSlew=enabled;state.hw3SlewRate=rate;
  const tgl=$('hw3-slew-tgl');if(tgl)tgl.checked=enabled;
  const inp=$('hw3-slew-rate');if(inp&&document.activeElement!==inp)inp.value=rate;
  setText('hw3-slew-meta',enabled?('On \u2022 '+rate+'%'):'Off');
  setText('hw3-slew-rate-hint',rate+'%/s (about '+(rate*0.6).toFixed(1)+' km/h/s at 60 km/h)');
  setText('hw3-slew-target',d.hw3OffsetTarget===undefined?'0':d.hw3OffsetTarget);
  setText('hw3-slew-last',d.hw3OffsetLast===undefined?'0':d.hw3OffsetLast);
  setText('hw3-slew-count',d.hw3SlewCount||0);
}
async function saveHw3Slew(){
  const tgl=$('hw3-slew-tgl'),inp=$('hw3-slew-rate'),st=$('hw3-slew-status');
  let rate=parseInt(inp.value,10);
  if(isNaN(rate)||rate<1||rate>25){st.textContent='Use 1-25, max 25';st.style.color='var(--err)';return;}
  const enabled=tgl.checked?'1':'0';
  st.textContent='Saving...';st.style.color='var(--tx3)';
  try{
    const body='hw3OffsetSlew='+enabled+'&hw3SlewRate='+rate;
    const r=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    const d=await r.json();
    if(!d.ok)throw new Error();
    state.hw3OffsetSlew=enabled==='1';state.hw3SlewRate=rate;
    st.textContent='Saved';st.style.color='var(--ok)';
    poll();
  }catch(e){st.textContent='Save failed';st.style.color='var(--err)';}
}

const hw3CustomMax=[45,60,75,90,105];
const hw3HighMax=[120,150,180];
function clampToSlot(v,max){v=parseInt(v,10);return isNaN(v)?0:Math.max(0,Math.min(max,v));}
function updateHw3SpeedControl(d){
  const cust=!!d.hw3CustomSpeed,hse=!!d.hw3HighSpeedEnable;
  const enc=parseInt(d.hw3WireEncoding,10)===0?0:1;
  const cTgl=$('hw3-cust-tgl');if(cTgl)cTgl.checked=cust;
  const hTgl=$('hw3-hs-tgl');if(hTgl)hTgl.checked=hse;
  const eSel=$('hw3-enc');if(eSel&&document.activeElement!==eSel)eSel.value=String(enc);
  const ct=Array.isArray(d.hw3CustomTarget)?d.hw3CustomTarget:[];
  for(let i=0;i<5;i++){const el=$('hw3-ct-'+i);if(el&&document.activeElement!==el&&ct[i]!==undefined)el.value=clampToSlot(ct[i],hw3CustomMax[i]);}
  const hs=Array.isArray(d.hw3HighSpeedTarget)?d.hw3HighSpeedTarget:[];
  for(let i=0;i<3;i++){const el=$('hw3-hs-'+i);if(el&&document.activeElement!==el&&hs[i]!==undefined)el.value=clampToSlot(hs[i],hw3HighMax[i]);}
  const flags=[];
  if(cust)flags.push('Custom');
  if(hse)flags.push('HighSpd');
  flags.push(enc?'PCT4':'KPH5');
  setText('hw3-speed-meta',(cust||hse)?flags.join(' \u2022 '):('Off \u2022 '+(enc?'PCT4':'KPH5')));
  const flKph=parseInt(d.fusedSpeedLimitKph,10)||0;
  setText('hw3-fused',flKph?String(flKph)+' km/h':((d.fusedSpeedLimitRaw==31)?'NONE':'SNA'));
  setText('hw3-stock-off',(d.hw3StockOffset===undefined)?'0':String(d.hw3StockOffset)+' km/h');
  setText('hw3-tgt-raw',d.hw3OffsetTarget===undefined?'0':d.hw3OffsetTarget);
}
async function saveHw3Speed(){
  const st=$('hw3-speed-status');
  const cust=$('hw3-cust-tgl').checked?'1':'0';
  const hse=$('hw3-hs-tgl').checked?'1':'0';
  const enc=$('hw3-enc').value==='0'?'0':'1';
  const parts=['hw3CustomSpeed='+cust,'hw3HighSpeedEnable='+hse,'hw3WireEncoding='+enc];
  for(let i=0;i<5;i++){const el=$('hw3-ct-'+i),v=clampToSlot(el.value,hw3CustomMax[i]);el.value=v;parts.push('hw3CustomT'+i+'='+v);}
  for(let i=0;i<3;i++){const el=$('hw3-hs-'+i),v=clampToSlot(el.value,hw3HighMax[i]);el.value=v;parts.push('hw3HighTarget'+i+'='+v);}
  st.textContent='Saving...';st.style.color='var(--tx3)';
  try{
    const r=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:parts.join('&')});
    const d=await r.json();
    if(!d.ok)throw new Error();
    st.textContent='Saved';st.style.color='var(--ok)';
    poll();
  }catch(e){st.textContent='Save failed';st.style.color='var(--err)';}
}

function updateLegacyMppControl(d){
  const sec=$('legacy-mpp-section');
  if(sec)sec.style.display=(state.hw===0)?'':'none';
  const en=!!d.legacyMppOverride,cust=!!d.legacyMppCustomEnable,hse=!!d.legacyMppHighSpeedEnable;
  const t=$('legacy-mpp-tgl');if(t)t.checked=en;
  const ct=$('legacy-mpp-cust-tgl');if(ct)ct.checked=cust;
  const ht=$('legacy-mpp-hs-tgl');if(ht)ht.checked=hse;
  const cArr=Array.isArray(d.legacyMppCustomTarget)?d.legacyMppCustomTarget:[];
  for(let i=0;i<5;i++){const el=$('legacy-mpp-ct-'+i);if(el&&document.activeElement!==el&&cArr[i]!==undefined)el.value=cArr[i];}
  const hArr=Array.isArray(d.legacyMppHighSpeedTarget)?d.legacyMppHighSpeedTarget:[];
  for(let i=0;i<3;i++){const el=$('legacy-mpp-hs-'+i);if(el&&document.activeElement!==el&&hArr[i]!==undefined)el.value=hArr[i];}
  setText('legacy-mpp-bus-raw',(d.legacyMppLastRaw!==undefined?d.legacyMppLastRaw:0)+(d.legacyMppLastRaw?' ('+(d.legacyMppLastRaw*5)+' km/h)':''));
  setText('legacy-mpp-sent-raw',(d.legacyMppLastSentRaw!==undefined?d.legacyMppLastSentRaw:0)+(d.legacyMppLastSentRaw?' ('+(d.legacyMppLastSentRaw*5)+' km/h)':''));
  const flags=[];
  if(en)flags.push('On');
  if(cust)flags.push('Custom');
  if(hse)flags.push('HighSpd');
  setText('legacy-mpp-meta',flags.length?flags.join(' \u2022 '):'Off');
}
const legacyCustomMax=[45,60,75,90,105];
const legacyHighMax=[120,150,155];
async function saveLegacyMpp(){
  const st=$('legacy-mpp-status');
  const en=$('legacy-mpp-tgl').checked?'1':'0';
  const cust=$('legacy-mpp-cust-tgl').checked?'1':'0';
  const hse=$('legacy-mpp-hs-tgl').checked?'1':'0';
  const parts=['legacyMppOverride='+en,'legacyMppCustomEnable='+cust,'legacyMppHighSpeedEnable='+hse];
  for(let i=0;i<5;i++){const v=parseInt($('legacy-mpp-ct-'+i).value,10);if(!isNaN(v))parts.push('legacyMppCustomT'+i+'='+Math.max(0,Math.min(legacyCustomMax[i],v)));}
  for(let i=0;i<3;i++){const v=parseInt($('legacy-mpp-hs-'+i).value,10);if(!isNaN(v))parts.push('legacyMppHighTarget'+i+'='+Math.max(0,Math.min(legacyHighMax[i],v)));}
  st.textContent='Saving...';st.style.color='var(--tx3)';
  try{
    const r=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:parts.join('&')});
    const d=await r.json();
    if(!d.ok)throw new Error();
    st.textContent='Saved';st.style.color='var(--ok)';
    poll();
  }catch(e){st.textContent='Save failed';st.style.color='var(--err)';}
}
async function pushLogging(){
  const body='eprn='+($('tgl-eprn').checked?'1':'0');
  try{await fetch('/logging',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});}catch(e){}
  if($('tgl-eprn').checked)pollLog();
  poll();
}

async function emergencyStop(){
  try{
    updateInjectButtons(false);
    state.can=false;
    setText('s-inj','BLOCKED');
    setClass('s-inj','stat-val v-err');
    await fetch('/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'can=0'});
  }catch(e){}
  poll();
}
async function resumeInj(){try{state.can=true;updateInjectButtons(true);await fetch('/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'hw='+state.hw+'&sp='+state.sp+'&spa='+(state.spAuto?'1':'0')+'&can=1'});}catch(e){}poll();}
async function toggleFsdTopButton(){if(state.can)await emergencyStop();else await resumeInj();}
async function reboot(){if(!await dashConfirm('Reboot device?','Reboot','Reboot'))return;try{await fetch('/reboot',{method:'POST'});}catch(e){}}

function fmtUp(s){
  if(s<60)return s+'s';
  if(s<3600)return Math.floor(s/60)+'m '+String(s%60).padStart(2,'0')+'s';
  return Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m';
}
function fmtBytes(n){
  n=Number(n)||0;
  if(n>=1048576)return (n/1048576).toFixed(n>=10485760?1:2)+' MB';
  if(n>=1024)return (n/1024).toFixed(n>=10240?0:1)+' KB';
  return n+' B';
}
function pct(used,total){
  total=Number(total)||0;used=Number(used)||0;
  return total>0?Math.max(0,Math.min(100,used*100/total)):0;
}
function setFill(id,value){
  const el=$(id);if(el)el.style.width=Math.max(0,Math.min(100,value||0))+'%';
}
function resetTaskStatsUi(){
  const tb=$('sys-task-rows');
  if(tb)tb.innerHTML='<tr><td colspan="5" class="v-dim">--</td></tr>';
}
function resetSystemStatusUi(){
  ['sys-chip','sys-cpu','sys-board','sys-temp','sys-reset','sys-heap','sys-largest','sys-minheap','sys-psram','sys-tasks','sys-flash','sys-spiffs','sys-rssi','sys-wifi-mode','sys-apclients','sys-ble','sys-wireless','sys-fw'].forEach(id=>setText(id,'--'));
  setText('sys-summary',trText('Monitoring off'));
  setText('sys-cpu-load',trText('off'));
  ['sys-cpu0-fill','sys-cpu1-fill','sys-heap-fill','sys-app-fill','sys-spiffs-fill'].forEach(id=>setFill(id,0));
  resetTaskStatsUi();
}
function startSystemMonitor(){
  if(systemStatusEnabled)return;
  systemStatusEnabled=true;
  const t=$('sys-monitor-tgl');if(t)t.checked=true;
  loadSystemStatus();
  loadTaskStats();
  systemStatusTimer=setInterval(loadSystemStatus,1000);
  taskStatsTimer=setInterval(loadTaskStats,2000);
}
function stopSystemMonitor(){
  systemStatusEnabled=false;
  const t=$('sys-monitor-tgl');if(t)t.checked=false;
  if(systemStatusTimer){clearInterval(systemStatusTimer);systemStatusTimer=null;}
  if(taskStatsTimer){clearInterval(taskStatsTimer);taskStatsTimer=null;}
  resetSystemStatusUi();
}
function toggleSystemMonitor(){
  const t=$('sys-monitor-tgl');
  if(t&&t.checked)startSystemMonitor();else stopSystemMonitor();
}
function initSystemMonitor(){
  stopSystemMonitor();
}
function escapeHtml(s){
  return String(s===undefined?'':s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}
function parseTaskStats(text){
  const rows=[];
  const lines=String(text||'').split(/\r?\n/);
  for(const line of lines){
    const m=line.match(/^\s*([0-9]+(?:\.[0-9]+)?)\s+(\S+)\s+(\d+)\s+(\d+)\s+(\S+)\s+(.+?)\s*$/);
    if(!m)continue;
    const name=m[6].trim();
    if(!name||name==='task')continue;
    rows.push({cpu:m[1],core:m[2],prio:m[3],stack:m[4],state:m[5],task:name});
  }
  return rows;
}
async function loadTaskStats(){
  if(!systemStatusEnabled)return;
  return runPoll('task_stats',async()=>{
    const tb=$('sys-task-rows');if(!tb)return;
    try{
      const r=await fetch('/task_stats',{cache:'no-store'});
      const text=await r.text();
      const rows=parseTaskStats(text).slice(0,10);
      if(!rows.length){resetTaskStatsUi();return;}
      tb.innerHTML=rows.map(x=>
        '<tr><td class="task-name" title="'+escapeHtml(x.task)+'">'+escapeHtml(x.task)+'</td>'+
        '<td class="task-core">'+escapeHtml(x.core)+'</td>'+
        '<td class="task-cpu">'+escapeHtml(x.cpu)+'%</td>'+
        '<td class="task-stack">'+escapeHtml(x.stack)+'</td>'+
        '<td class="task-state">'+escapeHtml(x.state)+'</td></tr>'
      ).join('');
    }catch(e){
      tb.innerHTML='<tr><td colspan="5" class="v-dim">'+trText('Task stats unavailable')+'</td></tr>';
    }
  });
}
async function loadSystemStatus(){
  if(!systemStatusEnabled)return;
  return runPoll('system_status',async()=>{
    try{
      const d=await fetchPollJson('/system_status',2500);
      const heapUsed=(d.heap_total||0)-(d.heap_free||0);
      const appUsed=d.app_used||0;
      const spiffsUsed=d.spiffs_used||0;
      setText('sys-summary',(d.module||d.chip||'ESP32')+' \u2022 '+(d.cores||'?')+' cores \u2022 '+(d.cpu_mhz||'?')+' MHz now');
      setText('sys-chip',(d.module||d.chip||'?')+' rev '+(d.revision===undefined?'?':d.revision)+' / '+(d.target||''));
      setText('sys-cpu',(d.cores||'?')+' cores \u2022 now '+(d.cpu_mhz||'?')+' MHz \u2022 max '+(d.cpu_max_mhz||240)+' MHz');
      if(d.cpu_load_valid){
        setText('sys-cpu-load','CPU0 '+(d.cpu0_load||0)+'% \u2022 CPU1 '+(d.cpu1_load||0)+'%');
        setFill('sys-cpu0-fill',d.cpu0_load||0);setFill('sys-cpu1-fill',d.cpu1_load||0);
      }else{
        setText('sys-cpu-load',trText('warming up'));
        setFill('sys-cpu0-fill',0);setFill('sys-cpu1-fill',0);
      }
      setText('sys-board','SRAM '+fmtBytes(d.sram_bytes)+' + RTC '+fmtBytes(d.rtc_sram_bytes)+' \u2022 ROM '+fmtBytes(d.rom_bytes));
      setText('sys-temp',d.temp_c===null||d.temp_c===undefined?trText('unavailable'):(Number(d.temp_c).toFixed(1)+' \u00B0C'));
      setText('sys-reset',d.reset||'?');
      setText('sys-heap',fmtBytes(d.heap_free)+' free / '+fmtBytes(d.heap_total)+' total');
      setText('sys-largest',fmtBytes(d.heap_largest));
      setText('sys-minheap',fmtBytes(d.heap_min));
      setText('sys-psram',(d.psram_total||0)?(fmtBytes(d.psram_free)+' free / '+fmtBytes(d.psram_total)+' total'):trText('not enabled'));
      setText('sys-tasks',(d.tasks||'?')+' tasks');
      setText('sys-flash',fmtBytes(d.flash_size)+' flash \u2022 app '+(d.app_label||'?')+' '+fmtBytes(appUsed||d.app_size)+' / '+fmtBytes(d.app_size));
      setText('sys-spiffs',d.spiffs_ok?(fmtBytes(spiffsUsed)+' used / '+fmtBytes(d.spiffs_total)):'SPIFFS '+trText('unavailable'));
      setText('sys-rssi',d.wifi_rssi===null||d.wifi_rssi===undefined?(d.wifi_connected?'?':'offline'):(d.wifi_rssi+' dBm'));
      setText('sys-wifi-mode',(d.wifi_mode||'?')+' \u2022 '+(d.wifi_connected?trText('STA online'):trText('STA offline'))+' \u2022 sleep '+(d.wifi_sleep?trText('on'):trText('off')));
      setText('sys-apclients',(d.ap_clients||0)+' client'+((d.ap_clients||0)===1?'':'s'));
      setText('sys-ble',(d.ble_supported?trText('supported'):trText('not supported'))+' \u2022 '+(d.ble_enabled?trText('enabled'):trText('firmware disabled')));
      setText('sys-wireless',(d.wifi_standard||'2.4GHz Wi-Fi')+' \u2022 '+(d.wifi_max_mbps||150)+' Mbps max \u2022 BLE 5 LE');
      setText('sys-fw',(d.mac||'--')+' \u2022 '+(d.firmware||'unknown')+' \u2022 IDF '+(d.idf||'?'));
      setFill('sys-heap-fill',pct(heapUsed,d.heap_total));
      setFill('sys-app-fill',appUsed?pct(appUsed,d.app_size):0);
      setFill('sys-spiffs-fill',pct(spiffsUsed,d.spiffs_total));
    }catch(e){
      setText('sys-summary','System status unavailable');
    }
  });
}
function fmtAgeMs(ms){
  if(ms<1000)return ms+' ms';
  if(ms<10000)return (ms/1000).toFixed(1)+' s';
  if(ms<60000)return Math.round(ms/1000)+' s';
  return fmtUp(Math.floor(ms/1000));
}
function toHex(n,p){return n.toString(16).toUpperCase().padStart(p,'0')}
function fmtProbeData(data,dlc){
  if(!Array.isArray(data)||!dlc)return '--';
  return data.slice(0,dlc).map(v=>toHex((v||0)&255,2)).join(' ');
}
function renderWriteProbe(p){
  const status=$('probe-status');
  if(!p||!p.active){
    status.textContent='No injected frame yet';
    status.className='probe-status v-dim';
    $('probe-tx-meta').textContent='--';
    $('probe-tx').textContent='--';
    $('probe-rx-meta').textContent='--';
    $('probe-rx').textContent='--';
    return;
  }
  const id='CAN 0x'+toHex((p.id||0)&0x7FF,3)+(p.mux>=0?' 路 mux '+p.mux:'');
  $('probe-tx-meta').textContent=id+' 路 '+fmtAgeMs(p.txa||0)+' ago';
  $('probe-tx').textContent=fmtProbeData(p.tx,p.txdlc);
  if(p.hasrx){
    $('probe-rx-meta').textContent=id+' 路 '+fmtAgeMs(p.rxa||0)+' ago';
    $('probe-rx').textContent=fmtProbeData(p.rx,p.rxdlc);
  }else{
    $('probe-rx-meta').textContent='No matching RX frame seen yet';
    $('probe-rx').textContent='--';
  }
  let text='Waiting for next matching bus frame';
  let cls='probe-status v-acc';
  if(p.state===2){text='Matching frame seen on bus';cls='probe-status v-ok';}
  else if(p.state===3){text='Latest bus frame differs from injected frame';cls='probe-status v-warn';}
  else if(p.state===4){text='Driver transmit failed';cls='probe-status v-err';}
  status.textContent=text;
  status.className=cls;
}

function renderEflg(e){
  const el=$('eflg-row');
  if(!e){el.innerHTML='<span class="eflg-pill eflg-ok">OK</span>';return;}
  let h='';
  if(e&0x20)h+='<span class="eflg-pill eflg-err">Bus-Off</span>';
  if(e&0x10)h+='<span class="eflg-pill eflg-warn">TX Passive</span>';
  if(e&0x08)h+='<span class="eflg-pill eflg-warn">RX Passive</span>';
  if(e&0x04)h+='<span class="eflg-pill eflg-warn">TX Warn</span>';
  if(e&0x02)h+='<span class="eflg-pill eflg-warn">RX Warn</span>';
  if(e&0xC0)h+='<span class="eflg-pill eflg-err">RX Overflow</span>';
  el.innerHTML=h||'<span class="eflg-pill eflg-ok">OK</span>';
}

function togglePause(){
  sniffPaused=!sniffPaused;
  syncSniffPauseButton();
  renderSniffer();
}

function renderSniffer(){
  updateSniffIdToggle();
  const filter=$('sniff-filter').value.trim().toLowerCase();
  const el=$('sniffer');
  let frames=sniffFrames;
  if(filter){
    const fid=parseInt(filter);
    if(!isNaN(fid))frames=frames.filter(f=>sniffWireId(f.id)===fid||sniffDbcId(f.id)===fid);
    else frames=frames.filter(f=>f.name&&f.name.toLowerCase().includes(filter));
  }
  $('sniff-count').textContent=frames.length+' frames';
  if(!frames.length){
    el.innerHTML='<div style="padding:20px;color:var(--tx3);text-align:center;font-size:12px">'+(sniffPaused?'Sniffer paused':'No frames')+'</div>';
    return;
  }
  const ADIds=new Set([1021,1016,921]);
  el.innerHTML=frames.slice(-30).reverse().map(f=>{
    const hex=Array.from({length:f.dlc},(_,i)=>toHex(f.data[i],2)).join(' ');
    const wireId=sniffWireId(f.id),dbcId=sniffDbcId(f.id),displayId=sniffDisplayId(f.id);
    const altId=sniffShowDbcIds?('Wire 0x'+toHex(wireId,3)):('DBC '+sniffBusLabel()+' 0x'+toHex(dbcId,3));
    return`<div class="sniff-row${ADIds.has(f.id)?' hi':''}">
      <span class="s-ts">${(f.ts/1000).toFixed(1)}s</span>
      <span class="s-id" title="${altId}">0x${toHex(displayId,3)}</span>
      <div><div class="s-data">${hex}</div>${f.name?`<div class="s-name">${f.name}</div>`:''}</div>
    </div>`;
  }).join('');
}

async function pollSniffer(){
  return runPoll('frames',async()=>{
    if(sniffPaused||!dashboardStatusOk)return;
    try{const d=await fetchPollJson('/frames',2500);sniffFrames=d.frames||[];renderSniffer();}catch(e){}
  });
}

// CAN pins + settings backup
async function loadCanPins(){
  try{
    const d=await fetchPollJson('/can_pins',2000);
    const tx=$('can-tx'),rx=$('can-rx'),st=$('can-pins-status'),hint=$('can-pins-hint');
    if(tx){tx.value=d.tx>=0?d.tx:'';tx.placeholder=d.tx>=0?'TX GPIO '+d.tx:'TX GPIO';}
    if(rx){rx.value=d.rx>=0?d.rx:'';rx.placeholder=d.rx>=0?'RX GPIO '+d.rx:'RX GPIO';}
    const label=(d.customized?'custom':'firmware default')+' TX='+d.tx+' RX='+d.rx;
    if(st){st.textContent=label;st.style.color=d.customized?'var(--acc)':'var(--tx3)';applyDashboardI18n(st);}
    if(hint){hint.textContent=d.customized?'Custom CAN pins saved in NVS. Reboot required after change.':'Using firmware default CAN pins. Save only if your hardware wiring differs.';applyDashboardI18n(hint);}
  }catch(e){const st=$('can-pins-status');if(st){st.textContent='unavailable';st.style.color='var(--err)';}}
}
async function saveCanPins(){
  const tx=parseInt($('can-tx').value,10),rx=parseInt($('can-rx').value,10),hint=$('can-pins-hint');
  if(isNaN(tx)||isNaN(rx)){if(hint){hint.textContent='Enter TX and RX GPIO';hint.style.color='var(--err)';}return;}
  if(!await dashConfirm('Save CAN pins TX='+tx+' RX='+rx+' and reboot? Wrong pins disable CAN.','Save CAN pins','Save'))return;
  try{
    const r=await fetch('/can_pins',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'tx='+tx+'&rx='+rx});
    const d=await r.json().catch(()=>({}));
    if(!r.ok||!d.ok)throw new Error(d.error||'save failed');
    if(hint){hint.textContent='Saved. Reboot required.';hint.style.color='var(--ok)';}
    loadCanPins();
  }catch(e){if(hint){hint.textContent=e.message||'Save failed';hint.style.color='var(--err)';}}
}
async function exportSettings(){
  const st=$('backup-status');
  if(st){st.textContent='Downloading...';st.style.color='var(--tx3)';}
  try{
    const r=await fetch('/settings_export',{cache:'no-store'});
    if(!r.ok)throw new Error('HTTP '+r.status);
    const blob=await r.blob();
    const url=URL.createObjectURL(blob);
    const a=document.createElement('a');
    a.href=url;
    a.download='evtools-backup.json';
    document.body.appendChild(a);a.click();a.remove();
    setTimeout(()=>URL.revokeObjectURL(url),2000);
    if(st){st.textContent='Downloaded';st.style.color='var(--ok)';}
  }catch(e){if(st){st.textContent='Download failed';st.style.color='var(--err)';}}
}
async function importSettings(event){
  const file=event&&event.target&&event.target.files?event.target.files[0]:null,st=$('backup-status');
  if(!file)return;
  if(st){st.textContent='Restoring...';st.style.color='var(--tx3)';}
  try{
    const text=await file.text();
    const r=await fetch('/settings_import',{method:'POST',headers:{'Content-Type':'application/json'},body:text});
    const d=await r.json().catch(()=>({}));
    if(!r.ok||!d.ok)throw new Error(d.error||'restore failed');
    if(st){st.textContent='Restored. Reboot required.';st.style.color='var(--ok)';}
  }catch(e){if(st){st.textContent=e.message||'Restore failed';st.style.color='var(--err)';}}
  if(event&&event.target)event.target.value='';
}

// OTA upload
function fileSelected(file){
  if(!file)return;
  otaFile=file;
  const drop=$('ota-drop');
  drop.querySelector('.ota-text').textContent=file.name;
  drop.querySelector('.ota-sub').textContent=(file.size/1024).toFixed(0)+' KB';
  $('ota-upload-btn').style.display='block';
}

function handleDrop(e){
  e.preventDefault();
  $('ota-drop').classList.remove('drag');
  const file=e.dataTransfer.files[0];
  if(file&&file.name.endsWith('.bin'))fileSelected(file);
}

function resetOtaCredentials(){
  localStorage.removeItem('otaU');
  localStorage.removeItem('otaP');
  otaUser='';
  otaPass='';
  const btn=$('ota-reset-btn');
  if(btn){
    btn.textContent='OTA Credentials Reset';
    setTimeout(()=>{btn.textContent='Reset OTA Credentials';},1500);
  }
}

async function uploadFirmware(){
  if(!otaFile)return;
  if(!otaUser){otaUser=prompt('OTA Username:')||'';localStorage.setItem('otaU',otaUser);}
  if(!otaPass){otaPass=prompt('OTA Password:')||'';localStorage.setItem('otaP',otaPass);}
  if(!otaUser||!otaPass)return;
  const prog=$('ota-progress');
  const fill=$('ota-fill');
  const status=$('ota-status');
  prog.style.display='block';
  $('ota-upload-btn').disabled=true;
  $('ota-upload-btn').textContent='Flashing...';

  const xhr=new XMLHttpRequest();
  xhr.upload.onprogress=e=>{
    if(e.lengthComputable){
      const pct=Math.round(e.loaded/e.total*100);
      fill.style.width=pct+'%';
      status.textContent='Uploading... '+pct+'%';
    }
  };
  xhr.onload=()=>{
    if(xhr.status===200){
      status.textContent='Done! Device is rebooting...';
      fill.style.width='100%';
      setTimeout(()=>window.location.reload(),5000);
    } else {
      status.textContent='Upload failed: '+xhr.status;
      status.style.color='var(--err)';
    }
    $('ota-upload-btn').disabled=false;
    $('ota-upload-btn').textContent='Flash Firmware';
  };
  xhr.onerror=()=>{
    status.textContent='Connection error';
    status.style.color='var(--err)';
    $('ota-upload-btn').disabled=false;
  };
  xhr.open('POST','/update',true,otaUser,otaPass);
  xhr.setRequestHeader('Content-Type','application/octet-stream');
  xhr.setRequestHeader('X-File-Name',otaFile.name);
  xhr.setRequestHeader('X-File-Size',otaFile.size);
  xhr.send(otaFile);
}

async function poll(){
  return runPoll('status',async()=>{
    try{
      const d=await fetchPollJson('/status',5000,true);
    const on=!!d.can,armed=!!d.ci,injecting=typeof d.ia==='undefined'?armed:!!d.ia,fpsVal=Number(d.fps||0);
    state.hw=d.hw;state.sp=clampProfileForHw(d.hw,d.sp);state.spAuto=typeof d.spAuto==='undefined'?state.spAuto:!!d.spAuto;state.can=armed;
    updateFsdControl(d);
    updateHw3SlewControl(d);
    updateHw3SpeedControl(d);
    updateLegacyMppControl(d);
    setClass('dot','sdot '+(d.txerr>5?'dot-warn':on?'dot-on':'dot-off'));
    const apActive=typeof d.apActive==='undefined'?!!d.AD:!!d.apActive;
    const adEnabled=typeof d.adEnabled==='undefined'?false:!!d.adEnabled;
    updateInjectButtons(armed);

    setText('s-can',on?'Active':'Offline');
    setClass('s-can','stat-val '+(on?'v-ok':'v-err'));
    setText('s-inj',injectionStatusLabel(injecting,armed,d.apGate,d));
    setClass('s-inj','stat-val '+(injecting?'v-ok':(armed&&d.apGate?'v-warn':'v-err')));
    setText('s-AD',apActive?'Active':'Inactive');
    setClass('s-AD','stat-val '+(apActive?'v-ok':'v-dim'));
    setText('s-fps',fpsVal.toFixed(1)+' Hz');
    setClass('s-fps','stat-val '+(fpsVal>5?'v-acc':'v-dim'));
    setText('s-rx',d.rx);
    setText('s-tx',d.tx);
    setText('s-txerr',d.txerr);
    setClass('s-txerr','stat-val '+(d.txerr>0?'v-warn':'v-dim'));
    setText('s-fd',d.fd||'--');
    setText('s-prof',profileDisplayName(d.hw,state.sp,state.spAuto));
    setText('s-soff',d.soff||'0');
    setText('s-up',fmtUp(d.up));
    setText('s-mcp-raw','EFLG: 0x'+toHex(d.eflg,2));
    const fpsFill=$('fps-fill');if(fpsFill)fpsFill.style.width=Math.min(fpsVal/20*100,100)+'%';
    setText('hw-badge',HW[d.hw]||'?');
    updateGtwBadge(d.gtwap);
    try{renderEflg(d.eflg);}catch(e){}
    try{renderWriteProbe(d.probe);}catch(e){}
    if(d.mux){for(let i=0;i<3;i++){setText('m'+i+'rx',d.mux[i].rx);setText('m'+i+'tx',d.mux[i].tx);const e=$('m'+i+'err');if(e){e.textContent=d.mux[i].err;e.style.color=d.mux[i].err>0?'var(--err)':'';}}}
    updateSniffIdToggle();
    const hwSeg=$('hw-seg');if(hwSeg)updSeg(hwSeg,d.hw,'hw-btn');updateHW4(d.hw);updateProfileControls(d.hw,state.sp,state.spAuto);
    const eprn=$('tgl-eprn');if(eprn&&typeof d.eprn!=='undefined')eprn.checked=d.eprn;
    if(!dashboardInitialLoaded){
      dashboardInitialLoaded=true;
      loadWifiNetworks();loadWifiStatus();loadApStatus();loadCanPins();loadGatewayDns();loadGatewayStatus();loadGatewayBlocked();
      if(canDebugEnabled)startCanDebugPolling();
    }
    }catch(e){}
  });
}

function colorLog(l){
  if(l.includes('AD=ON')||l.includes('AD active'))return'<span class="lf">'+l+'</span>';
  if(l.match(/\[HW[34]\]|\[LEGACY\]|\[HW3\]/))return'<span class="lh">'+l+'</span>';
  if(l.includes('ERR')||l.includes('FAIL'))return'<span class="le">'+l+'</span>';
  if(l.includes('[CFG]'))return'<span class="lc">'+l+'</span>';
  if(l.includes('[OK]')||l.includes('[BOOT]'))return'<span class="lf">'+l+'</span>';
  if(l.includes('[OTA]'))return'<span class="lo">'+l+'</span>';
  return l;
}
async function pollLog(){
  return runPoll('log',async()=>{
    if(!$('tgl-eprn').checked||!dashboardStatusOk)return;
    try{
      const d=await fetchPollJson('/log?since='+logSince,2000);
    if(d.seq)logSince=d.seq;
    if(!d.lines.length)return;
    const el=$('log');
    const newHtml=d.lines.map(colorLog).join('\n');
    if(el.textContent==='Waiting...'||el.textContent==='绛夊緟涓?..'||!el.dataset.logSeen)el.innerHTML=newHtml,el.dataset.logSeen='1';
    else el.innerHTML+='\n'+newHtml;
    // trim to 100 lines
    const lines=el.innerHTML.split('\n');
    if(lines.length>100)el.innerHTML=lines.slice(-100).join('\n');
    el.scrollTop=el.scrollHeight;
    }catch(e){}
  });
}

async function resetStats(){try{await fetch('/reset_stats',{method:'POST'});}catch(e){}poll();}

let recIsActive=false,recInterval=null;
async function toggleRec(){recIsActive?await stopRec():await startRec();}
async function startRec(ids){
  try{
    await fetch('/rec_start'+(ids?('?ids='+encodeURIComponent(ids)):''),{method:'POST'});
    recIsActive=true;
    const b=$('rec-btn');
    b.textContent='Stop Recording';
    b.style.borderColor='var(--err)';b.style.color='var(--err)';
    $('rec-dl').style.display='none';
    recInterval=setInterval(pollRec,800);
  }catch(e){}
}
async function stopRec(){
  clearInterval(recInterval);recIsActive=false;
  try{await fetch('/rec_stop',{method:'POST'});}catch(e){}
  const b=$('rec-btn');
  b.textContent='Start Recording';b.style.borderColor='';b.style.color='';
  await pollRec();
}
async function pollRec(){
  if(document.hidden)return;
  try{
    const d=await(await fetch('/rec_status')).json();
    const pct=Math.min(d.count/d.cap*100,100);
    $('rec-fill').style.width=pct+'%';
    $('rec-count').textContent=d.count+' / '+d.cap+' frames';
    if(d.active){
      $('rec-status').textContent='Recording...';$('rec-status').style.color='var(--err)';
      $('rec-meta').textContent='Recording...';
    } else {
      $('rec-meta').textContent=d.saved?d.count+' frames saved':'Idle';
      $('rec-status').textContent=d.saved?'Saved':'Ready';
      $('rec-status').style.color=d.saved?'var(--ok)':'';
      $('rec-dl').style.display=d.saved?'':'none';
      if(recIsActive){recIsActive=false;clearInterval(recInterval);const b=$('rec-btn');b.textContent='Start Recording';b.style.borderColor='';b.style.color='';}
    }
  }catch(e){}
}

// 鈹€鈹€ AP Hotspot management 鈹€鈹€
async function saveAP(){
  const ssid=$('ap-ssid').value,pass=$('ap-pass').value,hidden=$('ap-hidden').checked?'1':'0';
  if(!ssid){$('ap-status').textContent='Enter hotspot name';$('ap-status').style.color='var(--err)';return;}
  if(pass&&pass.length<8){$('ap-status').textContent='Password min 8 chars';$('ap-status').style.color='var(--err)';return;}
  try{const r=await fetch('/ap_config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)+'&hidden='+hidden});
    const d=await r.json();
    if(d.ok){$('ap-status').textContent=d.msg||'Saved! AP starts on CH1 and auto matches STA after WiFi connects.';$('ap-status').style.color='var(--ok)';$('ap-pass').value='';}
    else{$('ap-status').textContent=d.error||'Error';$('ap-status').style.color='var(--err)';}
  }catch(e){$('ap-status').textContent='Error';$('ap-status').style.color='var(--err)';}
}
async function loadApStatus(){
  return runPoll('ap_status',async()=>{
    if(!dashboardStatusOk)return;
    try{const d=await fetchPollJson('/ap_status',2000);
    if(d.ssid)$('ap-ssid').value=d.ssid;
    $('ap-clients').textContent=d.clients+' client'+(d.clients!==1?'s':'');
    if(typeof d.hidden!=='undefined')$('ap-hidden').checked=!!d.hidden;
    if($('ap-status')){
      const sync=d.last_channel_sync_ms?(' \u2022 sync '+(d.last_channel_sync_ok?'ok':'fail')+' CH'+(d.last_channel_sync_target||'?')):'';
      $('ap-status').textContent='AP CH'+(d.channel||'?')+' \u2022 auto match STA'+sync;
      $('ap-status').style.color='var(--tx3)';
    }
    if(d.stored){$('ap-stored').textContent='saved';$('ap-stored').style.color='var(--ok)';}
    else{$('ap-stored').textContent='firmware default';$('ap-stored').style.color='var(--tx3)';}
    }catch(e){}
  });
}
// 鈹€鈹€ WiFi management 鈹€鈹€
function toggleStaticIP(){
  $('static-fields').style.display=$('wifi-static').checked?'block':'none';
}
function rssiIcon(r){
  if(r>=-50) return '\u2587\u2587\u2587\u2587';
  if(r>=-60) return '\u2587\u2587\u2587\u2581';
  if(r>=-70) return '\u2587\u2587\u2581\u2581';
  return '\u2587\u2581\u2581\u2581';
}
function wifiAuthLabel(a){
  const n=Number(a);
  if(n===0)return 'OPEN';
  if(n===1)return 'WEP';
  if(n===2)return 'WPA';
  if(n===3)return 'WPA2';
  if(n===4)return 'WPA/WPA2';
  if(n===5)return 'ENT';
  if(n===6)return 'WPA3';
  if(n===7)return 'WPA2/WPA3';
  if(n===8)return 'WAPI';
  if(n===9)return 'WPA3-ENT';
  return 'AUTH'+(Number.isFinite(n)?n:'?');
}
async function scanWifi(){
  $('scan-btn').textContent='Scanning...';$('scan-btn').disabled=true;
  try{
    const r=await fetch('/wifi_scan');const d=await r.json();
    const el=$('wifi-nets');
    if(!d.networks.length){el.innerHTML='<div style="padding:8px;font-size:11px;color:var(--tx3);text-align:center">No networks found</div>';el.style.display='block';}
    else{el.innerHTML=d.networks.map(n=>'<div data-wifi-ssid="'+escapeHtml(n.ssid)+'" style="padding:6px 10px;cursor:pointer;display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--bd);font-size:12px" onmouseover="this.style.background=\'var(--bg)\'" onmouseout="this.style.background=\'\'"><span>'+(n.enc?'\uD83D\uDD12 ':'')+escapeHtml(n.ssid)+'</span><span style="color:var(--tx3);font-size:10px">'+rssiIcon(n.rssi)+' '+n.rssi+'dBm CH'+n.ch+' '+wifiAuthLabel(n.auth)+'</span></div>').join('');el.querySelectorAll('[data-wifi-ssid]').forEach(row=>row.onclick=()=>pickWifi(row.dataset.wifiSsid||''));el.style.display='block';}
  }catch(e){$('wifi-status').textContent='Scan failed';$('wifi-status').style.color='var(--err)';}
  $('scan-btn').textContent='Scan';$('scan-btn').disabled=false;
}
function pickWifi(ssid){
  $('wifi-ssid').value=ssid;$('wifi-nets').style.display='none';$('wifi-pass').focus();
}
let wifiSlotCache={count:0,max:4,active:-1,networks:[]};
let wifiStatusCache={};
function escapeHtml(s){return String(s||'').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;');}
function renderWifiSlots(){
  const list=$('wifi-saved-list'),wrap=$('wifi-add-wrap'),cnt=$('wifi-slot-count');
  if(!list)return;
  const nets=wifiSlotCache.networks||[];
  const max=wifiSlotCache.max||4;
  const active=wifiSlotCache.active;
  const connectedSsid=wifiStatusCache.connected?String(wifiStatusCache.ssid||''):'';
  const tryingIdx=(!wifiStatusCache.connected&&wifiStatusCache.connecting)?active:-1;
  cnt.textContent='('+nets.length+'/'+max+')';
  if(!nets.length){
    list.innerHTML='<div style="font-size:11px;color:var(--tx3);padding:6px 0">No networks saved.</div>';
  }else{
    list.innerHTML=nets.map(n=>{
      const isConnected=connectedSsid&&n.ssid===connectedSsid;
      const isTrying=n.idx===tryingIdx;
      const dotColor=isConnected?'var(--ok)':(isTrying?'var(--warn)':'var(--tx3)');
      const dot='<span title="'+(isConnected?'connected':(isTrying?'trying':'saved'))+'" style="display:inline-block;width:8px;height:8px;border-radius:50%;background:'+dotColor+';margin-right:6px"></span>';
      const tag=n.static?'<span style="font-size:10px;color:var(--tx3);margin-left:6px">[static]</span>':'';
      const state=isConnected?'<span style="font-size:10px;color:var(--ok);margin-left:6px">[connected]</span>':(isTrying?'<span style="font-size:10px;color:var(--warn);margin-left:6px">[trying]</span>':'');
      const connectLabel=isConnected?'Reconnect':'Connect';
      return '<div style="display:flex;align-items:center;gap:6px;padding:6px 0;border-bottom:1px solid var(--bd);font-size:12px">'+
        '<div style="flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">'+dot+escapeHtml(n.ssid)+tag+state+'</div>'+
        '<button class="sniff-btn" onclick="connectWifiSlot('+n.idx+')" style="padding:4px 8px;font-size:11px;border-color:var(--accBd);color:var(--acc)">'+trText(connectLabel)+'</button>'+
        '<button class="sniff-btn" onclick="editWifiSlot('+n.idx+')" style="padding:4px 8px;font-size:11px">'+trText('Edit')+'</button>'+
        '<button class="sniff-btn" onclick="deleteWifiSlot('+n.idx+')" style="padding:4px 8px;font-size:11px;background:var(--errBg);border-color:var(--errBd);color:var(--err)">'+trText('Delete')+'</button>'+
      '</div>';
    }).join('');
  }
  const editIdx=parseInt($('wifi-edit-idx').value,10);
  const canAdd=nets.length<max||editIdx>=0;
  wrap.style.display=canAdd?'':'none';
    $('wifi-save-btn').textContent=trText(editIdx>=0?'Save Changes':'Save & Connect');
}
async function loadWifiNetworks(){
  return runPoll('wifi_networks',async()=>{
    try{
      const d=await fetchPollJson('/wifi_networks',2000);
      wifiSlotCache=d;
      renderWifiSlots();
    }catch(e){}
  });
}
async function loadWifiStatus(){
  return runPoll('wifi_status',async()=>{
    try{const d=await fetchPollJson('/wifi_status',2000);
    wifiStatusCache=d;
    dashboardStaIp=d.connected&&d.ip?d.ip:'';
    if(typeof d.active==='number')wifiSlotCache.active=d.active;
    renderWifiSlots();
    const stName=d.wifi_status_name||('status '+(d.wifi_status===undefined?'?':d.wifi_status));
    const stCode=d.wifi_status===undefined?'?':d.wifi_status;
    const age=d.attempt_age_s===undefined?'':(' \u2022 '+d.attempt_age_s+'s');
    const reason=(d.disconnect_reason_name&&d.disconnect_reason_name!=='none')?(' \u2022 '+d.disconnect_reason_name+'('+d.disconnect_reason+')'):'';
    if(d.connected){
      setText('wifi-status',(d.ip&&d.ip!==location.hostname)?('Connected: '+(d.ssid||'')+' \u2022 '+d.ip+' \u2022 switch to that WiFi and open this IP'):('Connected: '+(d.ssid||'')+' \u2022 '+d.ip));
      $('wifi-status').style.color='var(--ok)';
    }
    else if(d.connecting&&d.ssid){
      setText('wifi-status','Connecting to '+d.ssid+age+' \u2022 '+stName+'('+stCode+')'+reason);$('wifi-status').style.color='var(--warn)';
    }
    else if(d.count>0){
      const retry=d.retry_in_s!==undefined?(' \u2022 retry in '+d.retry_in_s+'s'):'';
      setText('wifi-status',d.count+' saved'+retry+' \u2022 '+stName+'('+stCode+')'+reason);
      $('wifi-status').style.color='var(--tx3)';
    }
    else{
      setText('wifi-status','Not configured');
      $('wifi-status').style.color='var(--tx3)';
    }
    }catch(e){}
  });
}
async function connectWifiSlot(idx){
  const n=(wifiSlotCache.networks||[]).find(x=>x.idx===idx);
  if(!n)return;
  try{
    $('wifi-status').textContent=trText('Connecting to ')+n.ssid+'...';
    $('wifi-status').style.color='var(--acc)';
    const r=await fetch('/wifi_connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'idx='+idx});
    const d=await r.json().catch(()=>({}));
    if(!r.ok||d.ok===false)throw new Error(d.error||'connect failed');
    wifiSlotCache.active=idx;
    wifiStatusCache={connected:false,connecting:true,ssid:n.ssid,active:idx};
    renderWifiSlots();
    setTimeout(loadWifiStatus,500);
    setTimeout(loadWifiStatus,2500);
    setTimeout(loadWifiStatus,6500);
  }catch(e){
    $('wifi-status').textContent=e.message||'Connect failed';
    $('wifi-status').style.color='var(--err)';
  }
}
function editWifiSlot(idx){
  const n=(wifiSlotCache.networks||[]).find(x=>x.idx===idx);
  if(!n)return;
  $('wifi-edit-idx').value=idx;
  $('wifi-ssid').value=n.ssid;
  $('wifi-pass').value='';
  $('wifi-pass').placeholder=trText('Leave empty to keep current');
  $('wifi-static').checked=!!n.static;
  toggleStaticIP();
  if(n.static){
    $('wifi-ip').value=n.ip||'';
    $('wifi-gw').value=n.gw||'';
    $('wifi-mask').value=n.mask||'255.255.255.0';
    $('wifi-dns').value=n.dns||'';
  }
  renderWifiSlots();
  $('wifi-add-wrap').scrollIntoView({behavior:'smooth',block:'nearest'});
}
function clearWifiForm(){
  $('wifi-edit-idx').value=-1;
  $('wifi-ssid').value='';$('wifi-pass').value='';
  $('wifi-pass').placeholder=trText('Password');
  $('wifi-static').checked=false;toggleStaticIP();
  $('wifi-ip').value='';$('wifi-gw').value='';$('wifi-mask').value='255.255.255.0';$('wifi-dns').value='';
}
async function deleteWifiSlot(idx){
  const n=(wifiSlotCache.networks||[]).find(x=>x.idx===idx);
  if(!n)return;
  if(!await dashConfirm('Delete network "'+n.ssid+'"?','Delete WiFi','Delete'))return;
  try{
    await fetch('/wifi_delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'idx='+idx});
    if(parseInt($('wifi-edit-idx').value,10)===idx)clearWifiForm();
    loadWifiNetworks();loadWifiStatus();
  }catch(e){$('wifi-status').textContent='Delete failed';$('wifi-status').style.color='var(--err)';}
}
async function saveWifi(){
  const ssid=$('wifi-ssid').value,pass=$('wifi-pass').value;
  if(!ssid){$('wifi-status').textContent='Enter SSID';$('wifi-status').style.color='var(--err)';return;}
  const editIdx=parseInt($('wifi-edit-idx').value,10);
  const isEdit=editIdx>=0;
  if(!isEdit&&(wifiSlotCache.count||0)>=(wifiSlotCache.max||4)){
    $('wifi-status').textContent='Max '+(wifiSlotCache.max||4)+' networks';$('wifi-status').style.color='var(--err)';return;
  }
  let effectivePass=pass;
  if(isEdit&&!pass){
    effectivePass='';
  }
  let body='ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(effectivePass);
  if(isEdit)body+='&idx='+editIdx;
  if($('wifi-static').checked){
    body+='&static=1&ip='+encodeURIComponent($('wifi-ip').value)+'&gw='+encodeURIComponent($('wifi-gw').value)+'&mask='+encodeURIComponent($('wifi-mask').value)+'&dns='+encodeURIComponent($('wifi-dns').value);
  }
  try{
    const r=await fetch('/wifi_config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    const d=await r.json();
    if(!d.ok)throw new Error(d.error||'save failed');
    $('wifi-status').textContent='Connecting to '+ssid+'...';$('wifi-status').style.color='var(--acc)';
    clearWifiForm();
    loadWifiNetworks();
    setTimeout(loadWifiStatus,500);
    setTimeout(loadWifiStatus,2500);
    setTimeout(loadWifiStatus,5500);
  }catch(e){$('wifi-status').textContent=e.message||'Error';$('wifi-status').style.color='var(--err)';}
}
// 鈹€鈹€ STA-AP Gateway / DNS 鈹€鈹€
let gatewayDnsSaving=false;
let gatewayDnsBlackDirty=false,gatewayDnsWhiteDirty=false;
let gatewayDnsLastBlack=null,gatewayDnsLastWhite=null;
const gatewayDnsCacheKey='dashGatewayDnsStateV1';
const gatewayTeslaBlacklist='tesla.cn\ntesla.com\nteslamotors.com\ntesla.services';
const gatewayProfileSafeWhitelist='connman.vn.cloud.tesla.cn\nnav-prd-maps.tesla.cn\nmaps-cn-prd.go.tesla.services\nsignaling.vn.cloud.tesla.cn\napi-prd.vn.cloud.tesla.cn\nmedia-server-me.tesla.cn';
const gatewayProfileAggressiveWhitelist='connman.vn.cloud.tesla.cn\nnav-prd-maps.tesla.cn\nmaps-cn-prd.go.tesla.services\nsignaling.vn.cloud.tesla.cn\napi-prd.vn.cloud.tesla.cn\nmedia-server-me.tesla.cn\nhermes-prd.vn.cloud.tesla.cn\nhermes-stream-prd.vn.cloud.tesla.cn';
const gatewayProfileDescSafe='Conservative Mode: WiFi access / offline navigation / online navigation / China maps / WeChat notifications / Bluetooth music / voice assistant.';
const gatewayProfileDescAggressive='Aggressive Mode: WiFi access / offline navigation / online navigation / China maps / WeChat notifications / Bluetooth music / voice assistant / app vehicle control.';
function gatewayDnsEditing(id){
  const el=$(id);
  return el&&document.activeElement===el;
}
function initGatewayDnsEditing(){
  const black=$('gw-blacklist'),white=$('gw-whitelist');
  if(black&&!black.dataset.dirtyHooked){
    black.dataset.dirtyHooked='1';
    black.addEventListener('input',()=>{gatewayDnsBlackDirty=true;updateGatewayProfileButtons();});
  }
  if(white&&!white.dataset.dirtyHooked){
    white.dataset.dirtyHooked='1';
    white.addEventListener('input',()=>{gatewayDnsWhiteDirty=true;updateGatewayProfileButtons();});
  }
}
function updateGatewayTextarea(id,next,last,dirty,forceApply){
  const el=$(id);
  if(!el)return last;
  next=next||'';
  const remoteChanged=last!==null&&next!==last;
  if(!forceApply&&(dirty||gatewayDnsEditing(id))&&remoteChanged){
    const msg=$('gw-msg');
    if(msg){msg.textContent='Remote DNS list changed. Finish editing or save to overwrite.';msg.style.color='var(--warn)';applyDashboardI18n(msg);}
    return last;
  }
  if(!dirty&&el.value!==next)el.value=next;
  return next;
}
function normalizeGatewayList(v){
  return gatewayListItems(v).join('\n');
}
function gatewayListItems(v){
  const seen=new Set();
  const out=[];
  String(v||'').split(/[\s,;]+/).forEach(x=>{
    const item=x.trim().toLowerCase();
    if(item&&!seen.has(item)){seen.add(item);out.push(item);}
  });
  return out;
}
function gatewayListHasAll(current,template){
  const cur=new Set(gatewayListItems(current));
  return gatewayListItems(template).every(x=>cur.has(x));
}
function gatewayListHasAny(current,template){
  const cur=new Set(gatewayListItems(current));
  return gatewayListItems(template).some(x=>cur.has(x));
}
function mergeGatewayList(current,template){
  return gatewayListItems((current||'')+'\n'+(template||'')).join('\n');
}
function removeGatewayList(current,template){
  const drop=new Set(gatewayListItems(template));
  return gatewayListItems(current).filter(x=>!drop.has(x)).join('\n');
}
function updateGatewayProfileButtons(){
  const white=$('gw-whitelist')?$('gw-whitelist').value:'';
  const safe=gatewayListHasAll(white,gatewayProfileSafeWhitelist);
  const aggressive=gatewayListHasAll(white,gatewayProfileAggressiveWhitelist);
  const sb=$('gw-profile-safe'),ab=$('gw-profile-aggressive'),desc=$('gw-profile-desc');
  if(sb)sb.classList.toggle('active',safe&&!aggressive);
  if(ab)ab.classList.toggle('active',aggressive);
  if(desc){
    desc.textContent=aggressive?gatewayProfileDescAggressive:(safe?gatewayProfileDescSafe:'Custom DNS profile');
    applyDashboardI18n(desc);
  }
}
function readGatewayDnsCache(){
  try{
    const raw=localStorage.getItem(gatewayDnsCacheKey);
    return raw?JSON.parse(raw):null;
  }catch(e){return null;}
}
function writeGatewayDnsCache(d){
  try{
    if(!d||d.ok===false)return;
    localStorage.setItem(gatewayDnsCacheKey,JSON.stringify({
      blacklist:d.blacklist||'',whitelist:d.whitelist||'',
      upstream_mode:(d.upstream_mode!==undefined?d.upstream_mode:0),
      upstream_custom:d.upstream_custom||'',
      upstream_dhcp:d.upstream_dhcp||'',
      upstream_effective:d.upstream_effective||'',
      black_count:d.black_count||0,white_count:d.white_count||0,
      black_max:d.black_max||100,white_max:d.white_max||200
    }));
  }catch(e){}
}
function applyGatewayProfile(profile){
  initGatewayDnsEditing();
  const aggressive=profile==='aggressive';
  if($('gw-enabled'))$('gw-enabled').checked=true;
  if($('gw-blacklist'))$('gw-blacklist').value=gatewayTeslaBlacklist;
  if($('gw-whitelist')){
    let current=$('gw-whitelist').value;
    if(!aggressive&&gatewayListHasAny(current,'hermes-prd.vn.cloud.tesla.cn\nhermes-stream-prd.vn.cloud.tesla.cn'))
      current=removeGatewayList(current,'hermes-prd.vn.cloud.tesla.cn\nhermes-stream-prd.vn.cloud.tesla.cn');
    $('gw-whitelist').value=mergeGatewayList(current,aggressive?gatewayProfileAggressiveWhitelist:gatewayProfileSafeWhitelist);
  }
  gatewayDnsBlackDirty=true;gatewayDnsWhiteDirty=true;
  updateGatewayProfileButtons();
  saveGatewayDns().catch(()=>{});
}
function applyGatewayDnsState(d,opts){
  if(!d||!$('gw-enabled'))return;
  opts=opts||{};
  initGatewayDnsEditing();
  $('gw-enabled').checked=!!d.enabled;
  if($('gw-upstream-mode'))$('gw-upstream-mode').value=String(d.upstream_mode!==undefined?d.upstream_mode:0);
  if($('gw-upstream-custom')&&d.upstream_custom!==undefined)$('gw-upstream-custom').value=d.upstream_custom||'';
  toggleGatewayUpstreamCustom(d);
  if(d.blacklist!==undefined)gatewayDnsLastBlack=updateGatewayTextarea('gw-blacklist',d.blacklist,gatewayDnsLastBlack,opts.saved?false:gatewayDnsBlackDirty,!!opts.saved);
  if(d.whitelist!==undefined)gatewayDnsLastWhite=updateGatewayTextarea('gw-whitelist',d.whitelist,gatewayDnsLastWhite,opts.saved?false:gatewayDnsWhiteDirty,!!opts.saved);
  if(opts.saved){gatewayDnsBlackDirty=false;gatewayDnsWhiteDirty=false;}
  updateGatewayProfileButtons();
  var ce=$('gw-list-counts');
  if(ce){
    var bc=d.black_count||0,bm=d.black_max||100,wc=d.white_count||0,wm=d.white_max||200;
    ce.textContent='Whitelist '+wc+'/'+wm+' \u2022 Blacklist '+bc+'/'+bm;
    ce.style.color=(wc>=wm||bc>=bm)?'var(--err)':'var(--tx3)';
  }
  if(!opts.cached)writeGatewayDnsCache(d);
}
function setGatewayDiag(id,text,color){
  const el=$(id);if(!el)return;
  el.textContent=text;
  el.style.color=color||'var(--tx)';
}
function gatewayDnsSlowColor(d){
  if((d.dns_slow_2000ms||0)>0)return 'var(--err)';
  if((d.dns_slow_1000ms||0)>0||(d.dns_slow_500ms||0)>0)return 'var(--warn)';
  return 'var(--ok)';
}
function gatewayUpstreamModeLabel(v){
  v=String(v||'auto').toLowerCase();
  if(v==='ali')return 'Ali';
  if(v==='tencent')return 'Tencent';
  if(v==='custom')return trText('Custom');
  return trText('Auto');
}
function toggleGatewayUpstreamCustom(d){
  const sel=$('gw-upstream-mode'),inp=$('gw-upstream-custom'),hint=$('gw-upstream-hint');
  const mode=sel?Number(sel.value||0):0;
  document.querySelectorAll('.gateway-upstream-btn').forEach(btn=>{
    const active=Number(btn.dataset.mode||0)===mode;
    btn.classList.toggle('active',active);
    btn.setAttribute('aria-pressed',active?'true':'false');
  });
  if(inp){
    const custom=mode===3;
    inp.disabled=!custom;
    inp.style.opacity=custom?'1':'0.55';
  }
  if(hint){
    let text='Auto uses DHCP DNS from the connected WiFi; public DNS can avoid stale slow/fail counters from a bad router DNS.';
    if(mode===1)text='Using Ali DNS 223.5.5.5.';
    else if(mode===2)text='Using Tencent DNS 119.29.29.29.';
    else if(mode===3)text='Enter a custom upstream DNS IPv4 address.';
    hint.textContent=text;
    applyDashboardI18n(hint);
  }
}
function setGatewayUpstreamMode(mode,persist){
  const sel=$('gw-upstream-mode');
  if(sel)sel.value=String(mode);
  toggleGatewayUpstreamCustom();
  if(persist)saveGatewayDns().catch(()=>{});
}
async function loadGatewayStatus(){
  return runPoll('gateway_status',async()=>{
    try{
      const d=await fetchPollJson('/gateway_status',2000);
      if(!$('gw-status'))return;
      const clients=d.ap_clients||0;
      var statusText=(d.enabled?'Gateway ON':'Gateway OFF')+' \u2022 NAT '+(d.nat?'READY':'WAITING')+' \u2022 AP clients '+clients+' \u2022 blocked '+(d.blocked||0);
      if((d.dns_pending_full||0)>0)statusText+=' \u2022 pending FULL '+d.dns_pending_full;
      if(d.dns_resp_cache)statusText+=' \u2022 DNS cache '+(d.dns_resp_hits||0)+'/'+((d.dns_resp_hits||0)+(d.dns_resp_misses||0));
      $('gw-status').textContent=statusText;
      $('gw-status').style.color=!d.enabled?'var(--tx3)':((d.dns_pending_full||0)>0?'var(--err)':(d.nat?'var(--ok)':'var(--warn)'));
      const apCh=d.ap_channel?('CH'+d.ap_channel):'CH?';
      const staCh=d.sta_channel?('CH'+d.sta_channel):'CH?';
      const staRssi=(d.sta_rssi===null||d.sta_rssi===undefined)?'RSSI ?':('RSSI '+d.sta_rssi+' dBm');
      setGatewayDiag('gw-diag-ap',(d.ap_ip||'0.0.0.0')+' \u2022 '+apCh+' \u2022 '+clients+' client'+(clients===1?'':'s'),clients?'var(--ok)':'var(--tx)');
      setGatewayDiag('gw-diag-sta',d.sta_connected?((d.sta_ip||'0.0.0.0')+' \u2022 '+staRssi+' \u2022 '+staCh):'offline',''+(d.sta_connected?'var(--ok)':'var(--tx3)'));
      setGatewayDiag('gw-diag-nat',(d.napt_compiled?'compiled':'not compiled')+' / '+(d.nat?'READY':'WAITING'),d.nat?'var(--ok)':(d.enabled?'var(--warn)':'var(--tx3)'));
      setGatewayDiag('gw-diag-radio',apCh+' / STA '+staCh+' \u2022 '+(d.same_channel?'same':'cross'),d.same_channel?'var(--ok)':(d.sta_connected?'var(--warn)':'var(--tx3)'));
      setGatewayDiag('gw-diag-dns',(d.dns_task_active?'task':'no task')+' / '+(d.dns_bind_ok?'bind ok':'bind wait')+' / fd '+(d.dns_sock===undefined?'--':d.dns_sock),d.dns_task_active&&d.dns_bind_ok?'var(--ok)':'var(--warn)');
      setGatewayDiag('gw-diag-slow','last '+(d.dns_latency_last_ms||0)+' ms \u2022 avg '+(d.dns_latency_avg_ms||0)+' ms \u2022 >500/'+(d.dns_slow_500ms||0)+' >1s/'+(d.dns_slow_1000ms||0)+' >2s/'+(d.dns_slow_2000ms||0),gatewayDnsSlowColor(d));
      setGatewayDiag('gw-diag-pending',(d.dns_pending||0)+'/'+(d.dns_pending_capacity||64)+' \u2022 max '+(d.dns_pending_max||0)+' \u2022 full '+(d.dns_pending_full||0)+' \u2022 timeout '+(d.dns_timeouts||0),((d.dns_pending_full||0)>0||(d.dns_timeouts||0)>0)?'var(--err)':'var(--ok)');
      const upModeName=String(d.upstream_dns_mode_name||'auto').toLowerCase();
      const upMode=gatewayUpstreamModeLabel(upModeName);
      var upText=(d.upstream_dns||'none')+' \u2022 '+upMode;
      if(upModeName==='auto')upText+=' \u2022 DHCP '+(d.upstream_dns_dhcp||'none');
      else if(upModeName==='custom')upText+=' \u2022 custom '+(d.upstream_dns_custom||'none');
      upText+=' \u2022 fail '+(d.dns_upstream_fails||0);
      setGatewayDiag('gw-diag-upstream',upText,(d.dns_upstream_fails||0)>0?'var(--warn)':'var(--tx)');
      setGatewayDiag('gw-diag-clients',clients+' client'+(clients===1?'':'s'),clients?'var(--ok)':'var(--tx3)');
    }catch(e){
      if($('gw-status')){$('gw-status').textContent='Gateway not available';$('gw-status').style.color='var(--tx3)';}
      ['gw-diag-ap','gw-diag-sta','gw-diag-nat','gw-diag-radio','gw-diag-dns','gw-diag-slow','gw-diag-pending','gw-diag-upstream','gw-diag-clients'].forEach(id=>setGatewayDiag(id,'--','var(--tx3)'));
    }
  });
}
async function loadGatewayDns(force){
  if(gatewayDnsSaving)return;
  try{
    const r=await fetch('/gateway_dns');if(!r.ok)throw new Error('unavailable');
    const d=await r.json();
    applyGatewayDnsState(d);
  }catch(e){}
}
function loadGatewayDnsCached(){
  const d=readGatewayDnsCache();
  if(d)applyGatewayDnsState(d,{cached:true,saved:true});
}
async function resetGatewayDnsStats(){
  const msg=$('gw-msg');
  try{
    if(msg){msg.textContent='Resetting DNS stats...';msg.style.color='var(--tx3)';applyDashboardI18n(msg);}
    const r=await fetch('/gateway_dns_stats_reset',{method:'POST'});
    if(!r.ok)throw new Error('HTTP '+r.status);
    await r.json().catch(()=>({}));
    if(msg){msg.textContent='DNS stats reset';msg.style.color='var(--ok)';applyDashboardI18n(msg);}
    loadGatewayStatus();
  }catch(e){
    if(msg){msg.textContent=e&&e.message?e.message:'Error';msg.style.color='var(--err)';}
  }
}
async function saveGatewayDns(){
  const msg=$('gw-msg');
  try{
    gatewayDnsSaving=true;
    if(msg){msg.textContent='Saving...';msg.style.color='var(--tx3)';}
    const upstreamMode=$('gw-upstream-mode')?$('gw-upstream-mode').value:'0';
    const upstreamCustom=$('gw-upstream-custom')?$('gw-upstream-custom').value:'';
    const body='enabled='+($('gw-enabled').checked?1:0)+'&blacklist='+encodeURIComponent($('gw-blacklist').value)+'&whitelist='+encodeURIComponent($('gw-whitelist').value)+'&upstream_mode='+encodeURIComponent(upstreamMode)+'&upstream_custom='+encodeURIComponent(upstreamCustom);
    const r=await fetch('/gateway_dns',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    const d=await r.json().catch(()=>({}));
    if(!r.ok||d.ok===false)throw new Error(d.error||'save failed');
    applyGatewayDnsState(d,{saved:true});
    if(msg){msg.textContent='Saved';msg.style.color='var(--ok)';}
    loadGatewayStatus();setTimeout(loadGatewayBlocked,250);
  }catch(e){if(msg){msg.textContent=e.message||'Error';msg.style.color='var(--err)';}}
  finally{gatewayDnsSaving=false;}
}
function openGwBlockedModal(){loadGatewayBlocked();}
function closeGwBlockedModal(){}
function gwBlockedBackdrop(e){}
async function loadGatewayBlocked(){
  const list=$('gw-blocked-list');
  const sum=$('gw-blocked-summary');
  if(!list)return;
  try{
    const r=await fetch('/gateway_blocked');if(!r.ok)throw new Error('HTTP '+r.status);
    const d=await r.json();
    if(sum)sum.textContent=trText('Only non-blacklist domains can be added to the whitelist.')+' - '+(d.length||0)+' '+trText('items');
    if(!d.length){list.innerHTML='<div style="color:var(--tx3);text-align:center;padding:20px">'+trText('No blocked domains recorded')+'</div>';return;}
    list.innerHTML=d.map(x=>{
      const dom=escapeHtml(x.domain||'');
      const btn=x.blacklisted?'<span class="dns-state err">'+trText('Already in blacklist')+'</span>':
        x.whitelisted?'<span class="dns-state ok">'+trText('Already whitelisted')+'</span>':
        x.canWhitelist===false?'<span class="dns-state dim">'+trText('Not allowed')+'</span>':
        '<button class="sniff-btn modal-btn-primary" style="padding:4px 10px;font-size:11px" data-gw-domain="'+dom+'" onclick="addGatewayWhitelist(this.dataset.gwDomain)">'+trText('Add to Whitelist')+'</button>';
      return '<div class="dns-row">'+
        '<div class="dns-domain" title="'+dom+'">'+dom+'<span class="dns-count">x'+(x.count||0)+'</span></div><div>'+btn+'</div></div>';
    }).join('');
  }catch(e){
    list.innerHTML='<div style="color:var(--tx3);text-align:center;padding:20px">'+trText('DNS filter list unavailable')+': '+(e&&e.message?e.message:'fetch error')+'</div>';
  }
}
async function testGatewayDns(){
  const el=$('gw-test-result'),input=$('gw-test-domain');
  const domain=(input&&input.value?input.value:'').trim();
  if(!domain){if(el){el.textContent=trText('empty domain');el.style.color='var(--err)';}return;}
  try{
    const r=await fetch('/gateway_dns_test?domain='+encodeURIComponent(domain));
    if(!r.ok)throw new Error('HTTP '+r.status);
    const d=await r.json();
    const verdict=d.blocked?trText('would be blocked'):trText('would be allowed');
    const mode=trText('Blacklist');
    const reason=trText(d.reason||'');
    const gwState=d.enabled?'':' ('+trText('gateway disabled')+')';
    if(el){
      el.textContent=(d.domain||domain)+' - '+verdict+' - '+mode+' - '+reason+gwState;
      el.style.color=d.blocked?'var(--err)':'var(--ok)';
    }
  }catch(e){
    if(el){el.textContent=trText('DNS test failed')+': '+(e&&e.message?e.message:'network');el.style.color='var(--err)';}
  }
}
async function addGatewayWhitelist(domain){
  const msg=$('gw-blocked-msg')||$('gw-msg');
  try{
    gatewayDnsSaving=true;
    const r=await fetch('/gateway_whitelist_add',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'domain='+encodeURIComponent(domain)});
    const d=await r.json();
    if(!r.ok||!d.ok)throw new Error(d.error||'cannot add domain');
    applyGatewayDnsState(d);
    if(msg){msg.textContent=d.already?trText('Already whitelisted'):trText('Saved')+': '+domain;msg.style.color='var(--ok)';}
    setTimeout(loadGatewayBlocked,250);
  }catch(e){if(msg){msg.textContent=trText(e.message||'cannot add domain');msg.style.color='var(--err)';}}
  finally{gatewayDnsSaving=false;}
}
async function clearGatewayBlocked(){
  try{
    await fetch('/gateway_blocked_clear',{method:'POST'});
    const list=$('gw-blocked-list');if(list)list.innerHTML='<div style="color:var(--tx3);text-align:center;padding:20px">'+trText('Cleared')+'</div>';
    const sum=$('gw-blocked-summary');if(sum)sum.textContent='';
    loadGatewayStatus();
  }catch(e){}
}
startDashboardPolling();
document.addEventListener('visibilitychange',()=>{
  if(!dashboardVisible())return;
  poll();loadWifiStatus();loadApStatus();loadGatewayStatus();
  if(!networkPerformanceMode){loadWifiNetworks();loadGatewayBlocked();loadGatewayDns(true);}
  if(canDebugEnabled){pollLog();pollSniffer();pollRec();}
});
orderDashboardCards();initCardMinimizers();initSubsectionMinimizers();initSystemMonitor();positionCanDebugPanels();setCanDebugUi();updateHW4(1);updateProfileControls(1,0,true);updateSniffIdToggle();loadGatewayDnsCached();loadGatewayDns(true);loadGatewayStatus();if(!networkPerformanceMode)loadGatewayBlocked();poll();
</script>
</body>
</html>
)HTML";

