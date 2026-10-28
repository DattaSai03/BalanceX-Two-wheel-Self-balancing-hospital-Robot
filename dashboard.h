// BalanceX V3 control deck page, served at http://192.168.4.1 by BalanceX_V3.ino.
// Fully self-contained (inline CSS/JS/SVG): works with no internet connection.
#pragma once

static const char DASHBOARD_HTML[] = R"BXUI(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#03050b">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<title>BalanceX Control Deck</title>
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 48 48'%3E%3Cpath d='M24 3 42 13.5v21L24 45 6 34.5v-21Z' fill='%2307101f' stroke='%2325e6ff' stroke-width='3'/%3E%3Cpath d='M16 13 32 31M32 13 16 31' stroke='%23ff3fa4' stroke-width='4' stroke-linecap='round'/%3E%3C/svg%3E">
<style>
:root{
  --bg:#03050b;--ink:#eaf5ff;--mut:#8094b3;--dim:#4b5b78;
  --cy:#25e6ff;--bl:#3f7cff;--vi:#8f5bff;--pk:#ff3fa4;--ok:#35ffa0;--am:#ffb648;--rd:#ff3b5c;
  --line:rgba(120,190,255,.14);
  --acc:var(--cy);
  --fu:"Rajdhani","Orbitron","Exo 2","Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;
  --fm:ui-monospace,"SF Mono","Roboto Mono","Cascadia Mono",Menlo,Consolas,monospace;
  --dockH:96px;
}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
html{background:var(--bg)}
body{min-height:100vh;color:var(--ink);font-family:var(--fu);background:var(--bg);overflow-x:hidden;
  padding:calc(10px + env(safe-area-inset-top)) 0 calc(var(--dockH) + 28px + env(safe-area-inset-bottom));-webkit-font-smoothing:antialiased}
button{font:inherit;color:inherit;border:0;background:none;cursor:pointer;touch-action:manipulation;-webkit-user-select:none;user-select:none}
svg.i{width:18px;height:18px;flex:none;fill:none;stroke:currentColor;stroke-width:1.8;stroke-linecap:round;stroke-linejoin:round}
body[data-st="0"]{--acc:var(--cy)} body[data-st="1"]{--acc:var(--am)} body[data-st="2"]{--acc:var(--ok)} body[data-st="3"]{--acc:var(--rd)}

/* ---------- background ---------- */
.bg{position:fixed;inset:0;z-index:-1;overflow:hidden;background:radial-gradient(120% 70% at 50% -12%,#10204a 0%,#070c1c 42%,var(--bg) 74%)}
.aur{position:absolute;inset:-25%;background:
  radial-gradient(38% 28% at 16% 22%,rgba(37,230,255,.16),transparent 70%),
  radial-gradient(34% 30% at 88% 30%,rgba(255,63,164,.12),transparent 70%),
  radial-gradient(52% 34% at 50% 100%,rgba(63,124,255,.20),transparent 70%);animation:aur 16s ease-in-out infinite alternate}
@keyframes aur{to{transform:translate3d(4%,3%,0) scale(1.1)}}
.floor{position:absolute;left:-60%;right:-60%;bottom:0;height:52%;perspective:300px;perspective-origin:50% 0;
  -webkit-mask-image:linear-gradient(transparent,#000 45%);mask-image:linear-gradient(transparent,#000 45%)}
.plane{position:absolute;left:0;right:0;top:0;height:320%;transform-origin:50% 0;transform:rotateX(74deg);
  background-image:linear-gradient(rgba(37,230,255,.30) 1px,transparent 1px),linear-gradient(90deg,rgba(37,230,255,.22) 1px,transparent 1px);
  background-size:64px 64px;animation:grid 9s linear infinite}
@keyframes grid{from{transform:rotateX(74deg) translateY(-64px)}to{transform:rotateX(74deg) translateY(0)}}
body[data-md="1"][data-st="2"] .plane{animation-duration:1.6s}
body[data-md="2"][data-st="2"] .plane{animation-duration:1.6s;animation-direction:reverse}
body[data-st="3"] .plane,body.off .plane{animation-play-state:paused}
.stars{position:absolute;inset:0;opacity:.55;background-image:
  radial-gradient(1px 1px at 12% 18%,#fff,transparent),radial-gradient(1px 1px at 72% 9%,#bfe9ff,transparent),
  radial-gradient(1.5px 1.5px at 38% 31%,#fff,transparent),radial-gradient(1px 1px at 86% 42%,#fff,transparent),
  radial-gradient(1px 1px at 22% 47%,#9fdcff,transparent),radial-gradient(1.5px 1.5px at 58% 14%,#fff,transparent),
  radial-gradient(1px 1px at 93% 22%,#ffd6f0,transparent),radial-gradient(1px 1px at 5% 35%,#fff,transparent)}
.scan{position:absolute;inset:0;background:repeating-linear-gradient(rgba(255,255,255,.022) 0 1px,transparent 1px 3px);pointer-events:none}

/* ---------- layout ---------- */
.wrap{max-width:1180px;margin:0 auto;padding:0 12px;display:grid;gap:12px;grid-template-columns:minmax(0,1fr)}
@media(min-width:780px){.wrap{grid-template-columns:minmax(0,1.25fr) minmax(0,1fr);padding:0 22px}
  .top,.status,.stats{grid-column:1/-1}.hero{min-height:470px}}
.card{position:relative;border-radius:20px;padding:14px;
  background:linear-gradient(155deg,rgba(24,38,68,.78),rgba(8,13,26,.84) 60%);
  border:1px solid var(--line);
  box-shadow:0 24px 50px -24px rgba(0,0,0,.9),inset 0 1px 0 rgba(255,255,255,.06),inset 0 0 0 1px rgba(255,255,255,.015);
  animation:rise .7s cubic-bezier(.2,.8,.2,1) both;animation-delay:calc(var(--i,0)*70ms)}
.card::before,.card::after{content:"";position:absolute;width:16px;height:16px;border:2px solid var(--acc);opacity:.55;pointer-events:none;transition:border-color .4s}
.card::before{top:-1px;left:-1px;border-right:0;border-bottom:0;border-radius:20px 0 0 0}
.card::after{bottom:-1px;right:-1px;border-left:0;border-top:0;border-radius:0 0 20px 0}
@keyframes rise{from{opacity:0;transform:translateY(18px) scale(.98)}}
.ch{display:flex;align-items:center;gap:8px;font-size:13px;font-weight:700;letter-spacing:.18em;color:#cfe6ff;text-transform:uppercase}
.ch svg.i{color:var(--cy);filter:drop-shadow(0 0 6px rgba(37,230,255,.7))}
.ch em{font-style:normal;font-weight:500;color:var(--mut);letter-spacing:.08em;text-transform:none;font-size:12px}
.ch .sp{flex:1}

/* ---------- header ---------- */
.top{display:flex;align-items:center;gap:12px;padding:2px 2px 0}
.logo{width:46px;height:46px;flex:none;filter:drop-shadow(0 0 10px rgba(37,230,255,.55))}
.logo .hx{stroke-dasharray:120;stroke-dashoffset:120;animation:draw 1.4s .2s ease forwards}
.logo .xx{stroke-dasharray:46;stroke-dashoffset:46;animation:draw 1s .9s ease forwards}
@keyframes draw{to{stroke-dashoffset:0}}
.wm{flex:1;min-width:0}
.name{font-size:25px;font-weight:800;letter-spacing:.2em;line-height:1;white-space:nowrap;
  background:linear-gradient(90deg,#fff 0%,#bfefff 60%,#7fdcff);-webkit-background-clip:text;background-clip:text;color:transparent}
.name span{background:linear-gradient(135deg,var(--cy),var(--pk));-webkit-background-clip:text;background-clip:text;color:transparent;
  filter:drop-shadow(0 0 8px rgba(255,63,164,.6));margin-left:.06em}
.tag{font-size:10.5px;letter-spacing:.32em;color:var(--mut);margin-top:5px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.link{display:flex;align-items:center;gap:7px;padding:7px 11px;border-radius:999px;font-size:11px;font-weight:700;letter-spacing:.14em;
  background:rgba(53,255,160,.08);border:1px solid rgba(53,255,160,.35);color:var(--ok);white-space:nowrap}
.link svg.i{width:15px;height:15px}
.link b{font-family:var(--fm);font-weight:500;letter-spacing:0;color:#9fffd0;opacity:.8}
.link i{width:7px;height:7px;border-radius:50%;background:currentColor;box-shadow:0 0 10px currentColor;animation:blink 1.2s infinite}
@keyframes blink{50%{opacity:.25}}
body.off .link,body.wait .link{color:var(--rd);background:rgba(255,59,92,.08);border-color:rgba(255,59,92,.4)}
body.wait .link{color:var(--am);background:rgba(255,182,72,.08);border-color:rgba(255,182,72,.4)}

/* ---------- status ---------- */
.status{display:grid;grid-template-columns:auto auto;justify-content:space-between;align-items:center;gap:10px 10px}
.status .smsg{grid-column:1/-1;order:3}
.status .prog{order:4}.status .rst{order:5}
@media(min-width:780px){.status{grid-template-columns:auto minmax(0,1fr) auto;justify-content:stretch}.status .smsg{grid-column:auto;order:0}}
.chip{display:inline-flex;align-items:center;gap:8px;padding:8px 13px;border-radius:12px;font-weight:800;font-size:13px;letter-spacing:.16em;white-space:nowrap}
.stc{color:var(--acc);background:rgba(255,255,255,.05);border:1px solid rgba(255,255,255,.22);box-shadow:0 0 22px -6px var(--acc);text-shadow:0 0 10px var(--acc)}
.stc{background:color-mix(in srgb,var(--acc) 12%,transparent);border:1px solid color-mix(in srgb,var(--acc) 45%,transparent);
  box-shadow:0 0 22px -6px var(--acc),inset 0 0 12px -6px var(--acc);text-shadow:0 0 10px var(--acc)}
.stc i{width:9px;height:9px;border-radius:50%;background:currentColor;box-shadow:0 0 12px currentColor}
body[data-st="2"] .stc i{animation:beat 1s infinite}
@keyframes beat{0%,100%{transform:scale(1)}30%{transform:scale(1.6)}}
.smsg{font-size:14px;font-weight:600;color:#cbd9ee;letter-spacing:.02em;min-width:0}
.smsg b{color:var(--acc);font-weight:700}
.mdc{color:#d7e8ff;border:1px solid var(--line);background:rgba(255,255,255,.03);font-size:12px}
.mdc svg.i{color:var(--mc,var(--cy))}
.prog{grid-column:1/-1;height:6px;border-radius:9px;background:rgba(255,255,255,.06);overflow:hidden;display:none}
.prog i{display:block;height:100%;width:0;border-radius:inherit;background:linear-gradient(90deg,var(--acc),#fff);box-shadow:0 0 14px var(--acc);transition:width .25s}
body[data-st="0"] .prog,body[data-st="1"] .prog{display:block}
.rst{grid-column:1/-1;display:none;align-items:center;justify-content:center;gap:10px;height:50px;border-radius:14px;font-weight:800;letter-spacing:.16em;font-size:14px;
  color:#04131a;background:linear-gradient(180deg,#7ff3ff,#19c7f0);box-shadow:0 5px 0 #0b6f8a,0 12px 28px -8px rgba(37,230,255,.7);transform:translateY(-2px);transition:transform .1s,box-shadow .1s}
.rst:active{transform:translateY(3px);box-shadow:0 0 0 #0b6f8a,0 6px 16px -8px rgba(37,230,255,.7)}
.rst svg.i{width:20px;height:20px;stroke-width:2.2}
body[data-st="3"] .status .rst{display:flex}

/* ---------- hero / 3D twin ---------- */
.hero{padding:14px 0 0;overflow:hidden;min-height:400px;display:flex;flex-direction:column}
.hero .ch{padding:0 14px}
.hint{font-size:10px;letter-spacing:.2em;color:var(--dim);font-weight:700}
.stage{position:relative;flex:1;min-height:330px;perspective:900px;perspective-origin:50% 35%;touch-action:pan-y;cursor:grab;overflow:hidden}
.stage:active{cursor:grabbing}
.world{position:absolute;left:calc(50% - 52px);top:calc(50% + 66px);transform-style:preserve-3d;will-change:transform}
.world div,.world i{position:absolute;left:0;top:0;display:block;transform-style:preserve-3d}
.fc{backface-visibility:hidden;background:var(--c);border:1px solid var(--e,transparent)}
.fc.t{box-shadow:inset 0 0 0 300px rgba(255,255,255,.13)}
.fc.r{box-shadow:inset 0 0 0 300px rgba(0,0,0,.12)}
.fc.l,.fc.k{box-shadow:inset 0 0 0 300px rgba(0,0,0,.34)}
.fc.b{box-shadow:inset 0 0 0 300px rgba(0,0,0,.55)}
.pl{--c:rgba(16,36,66,.92);--e:rgba(37,230,255,.75)}
.pl .fc.t{background:linear-gradient(135deg,rgba(40,90,150,.55),rgba(14,30,58,.9)),repeating-linear-gradient(90deg,transparent 0 14px,rgba(37,230,255,.12) 14px 15px)}
.post{--c:linear-gradient(90deg,#5d6f88,#dfe9f7,#6b7d96)}
.mot{--c:linear-gradient(#26324a,#141b2a);--e:rgba(255,255,255,.12)}
.mot .fc.r,.mot .fc.l{background:radial-gradient(circle,#d8b25a 0 30%,#7a5a1c 32% 60%,#1a2133 62%)}
.drv{--c:#c0153f;--e:rgba(255,110,140,.8)}
.drv .fc.t{background:repeating-linear-gradient(90deg,#20232b 0 3px,#3c4150 3px 6px)}
.pcb{--c:#0e1a17;--e:rgba(53,255,160,.85)}
.pcb .fc.t{background:linear-gradient(90deg,#0e1a17 0 62%,#c9ced8 62% 92%,#0e1a17 92%);box-shadow:inset 0 0 12px rgba(53,255,160,.35)}
.imu{--c:#1f4fd8;--e:rgba(140,190,255,.9)}
.bat{--c:linear-gradient(#141a26,#0a0e16);--e:rgba(255,255,255,.14)}
.bat .fc.f,.bat .fc.k{background:linear-gradient(#141a26,#0a0e16);position:absolute}
.bat .fc.f::after{content:"";position:absolute;left:10%;right:10%;top:44%;height:4px;border-radius:4px;background:var(--acc);box-shadow:0 0 14px var(--acc),0 0 30px var(--acc);transition:background .4s,box-shadow .4s}
.son{--c:#0f56b8;--e:rgba(130,190,255,.95)}
.buzz{--c:#121520;--e:rgba(255,182,72,.4)}
body.buz .buzz{--c:#ffb648;--e:#fff;filter:drop-shadow(0 0 10px #ffb648)}
.eye i{width:20px;height:20px;margin:-10px 0 0 -10px;border-radius:50%;background:radial-gradient(circle,#e8eef8 0 18%,#8a96aa 20% 55%,#3b4458 58%);border:2px solid #c9d3e2}
.eye i.c{background:radial-gradient(circle,#0b0f17 0 35%,#3b4458 38%),repeating-radial-gradient(circle,#9aa6ba 0 1px,#5c677c 1px 2px);box-shadow:0 0 12px rgba(37,230,255,.7);border-color:#e7eef9}
.wheel i{width:84px;height:84px;margin:-42px 0 0 -42px;border-radius:50%}
.wheel i.rg{border:6px solid #0d121c;box-shadow:0 0 0 1px rgba(37,230,255,.22)}
.wheel i.cap{border:9px solid #10151f;background:
  radial-gradient(circle,#f4f8ff 0 7%,var(--wc) 8% 15%,#121a29 16% 23%,transparent 24%),
  repeating-conic-gradient(from 0deg,#2a3650 0 14deg,#5b6b88 14deg 18deg,#2a3650 18deg 60deg),#1a2234;
  box-shadow:0 0 0 2px var(--wc),0 0 18px -2px var(--wc),inset 0 0 0 3px #232d41}
.wheel i.cap::after{content:"";position:absolute;left:50%;top:-9px;width:6px;height:9px;margin-left:-3px;background:var(--wc);border-radius:2px;box-shadow:0 0 8px var(--wc)}
.wL{--wc:var(--cy)}.wR{--wc:var(--pk)}
.ground{width:640px;height:640px;margin:-320px 0 0 -320px;pointer-events:none}
.ground svg{width:100%;height:100%;overflow:visible}
.beam i{width:var(--bw);height:var(--L);margin-left:calc(var(--bw)/-2);transform-origin:50% 0;
  clip-path:polygon(50% 0,100% 100%,0 100%);background:linear-gradient(rgba(37,230,255,.55),rgba(37,230,255,0));transition:opacity .3s}
body[data-zone="1"] .beam i{background:linear-gradient(rgba(255,182,72,.7),rgba(255,182,72,0))}
body[data-zone="2"] .beam i{background:linear-gradient(rgba(255,59,92,.8),rgba(255,59,92,0))}
.wall{width:112px;height:86px;margin-left:-56px;border-radius:6px;transition:opacity .35s;
  background:linear-gradient(rgba(37,230,255,.05),rgba(37,230,255,.22));border:2px solid var(--cy);box-shadow:0 0 30px rgba(37,230,255,.5),inset 0 0 26px rgba(37,230,255,.4)}
.wall b{position:absolute;left:0;right:0;top:14px;text-align:center;font:800 19px/1 var(--fm);color:#fff;text-shadow:0 0 12px var(--cy)}
.wall s{position:absolute;left:0;right:0;bottom:12px;text-align:center;font:700 10px/1 var(--fu);letter-spacing:.25em;color:#dff;text-decoration:none}
body[data-zone="1"] .wall{border-color:var(--am);background:linear-gradient(rgba(255,182,72,.06),rgba(255,182,72,.28));box-shadow:0 0 34px rgba(255,182,72,.6),inset 0 0 26px rgba(255,182,72,.45)}
body[data-zone="1"] .wall b{text-shadow:0 0 12px var(--am)}
body[data-zone="2"] .wall{border-color:var(--rd);background:repeating-linear-gradient(-45deg,rgba(255,59,92,.42) 0 10px,rgba(255,59,92,.12) 10px 20px);box-shadow:0 0 40px rgba(255,59,92,.75),inset 0 0 26px rgba(255,59,92,.5)}
body[data-zone="2"] .wall b{text-shadow:0 0 12px var(--rd)}
.hud{position:absolute;left:14px;top:46px;pointer-events:none;z-index:2}
.hud .l{font-size:11px;letter-spacing:.3em;color:var(--mut);font-weight:700;display:flex;align-items:center;gap:6px}
.hud .l svg.i{width:14px;height:14px;color:var(--acc)}
.big{font-family:var(--fm);font-size:42px;font-weight:700;line-height:1.05;letter-spacing:-.02em;color:#fff;text-shadow:0 0 22px rgba(37,230,255,.5);text-shadow:0 0 22px color-mix(in srgb,var(--acc) 70%,transparent)}
.big small{font-size:22px;color:var(--acc);margin-left:2px}
.hud .s{display:flex;gap:14px;font-size:11px;letter-spacing:.14em;color:var(--mut);font-weight:700;margin-top:2px}
.hud .s b{font-family:var(--fm);color:#dcecff;font-weight:600;letter-spacing:0}
.hud2{position:absolute;right:14px;top:46px;text-align:right;pointer-events:none;z-index:2;display:flex;flex-direction:column;gap:6px;align-items:flex-end}
.tagx{font-size:10px;font-weight:800;letter-spacing:.2em;padding:5px 8px;border-radius:8px;border:1px solid var(--line);color:var(--mut);background:rgba(3,6,14,.55)}
.tagx.on{color:var(--ok);border-color:rgba(53,255,160,.45);box-shadow:0 0 14px -4px var(--ok)}
.tagx.wr{color:var(--am);border-color:rgba(255,182,72,.5)}
.tagx.bd{color:var(--rd);border-color:rgba(255,59,92,.55);box-shadow:0 0 14px -4px var(--rd)}
.att{display:block;width:100%;height:74px;margin-top:-6px}
.att text{font:700 9px var(--fm);fill:var(--mut)}

/* ---------- sonar ---------- */
.rbox{position:relative;margin-top:8px}
.radar{display:block;width:100%;height:auto;overflow:visible}
.radar .sw{transform-box:view-box;transform-origin:160px 182px;animation:sweep 3.4s ease-in-out infinite}
@keyframes sweep{0%,100%{transform:rotate(-44deg)}50%{transform:rotate(44deg)}}
body.off .radar .sw,body.snoff .radar .sw{animation-play-state:paused;opacity:.25}
.radar .rp{transform-box:fill-box;transform-origin:center;opacity:0}
.radar .rp.go{animation:rip .9s ease-out}
@keyframes rip{from{opacity:.9;transform:scale(.3)}to{opacity:0;transform:scale(2.6)}}
.radar text{font:700 9px var(--fm);fill:#7f93b0}
.obs{transition:stroke .3s}
.srow{display:flex;align-items:flex-end;justify-content:space-between;gap:10px;margin-top:-4px}
.dist{font-family:var(--fm);font-size:44px;font-weight:700;line-height:1;color:#fff;text-shadow:0 0 20px rgba(37,230,255,.55);white-space:nowrap}
.dist small{font-size:18px;color:var(--cy);margin-left:4px}
.dsub{font-size:11px;letter-spacing:.2em;color:var(--mut);font-weight:700;margin-top:4px}
.zone{padding:8px 12px;border-radius:12px;font-size:12px;font-weight:800;letter-spacing:.14em;text-align:center;white-space:nowrap;
  color:var(--ok);border:1px solid rgba(53,255,160,.45);background:rgba(53,255,160,.08);box-shadow:0 0 18px -6px var(--ok)}
body[data-zone="1"] .zone{color:var(--am);border-color:rgba(255,182,72,.55);background:rgba(255,182,72,.1);box-shadow:0 0 20px -4px var(--am)}
body[data-zone="2"] .zone{color:#fff;border-color:var(--rd);background:rgba(255,59,92,.35);box-shadow:0 0 26px -2px var(--rd);animation:blink .7s infinite}
body[data-zone="1"] .dist{text-shadow:0 0 20px rgba(255,182,72,.6)} body[data-zone="1"] .dist small{color:var(--am)}
body[data-zone="2"] .dist{text-shadow:0 0 22px rgba(255,59,92,.8)} body[data-zone="2"] .dist small{color:var(--rd)}
body.snoff .zone{color:var(--mut);border-color:var(--line);background:rgba(255,255,255,.03);box-shadow:none}
.bz{display:inline-flex;align-items:center;gap:6px;font-size:10.5px;font-weight:800;letter-spacing:.16em;padding:5px 9px;border-radius:9px;color:var(--dim);border:1px solid var(--line);text-transform:uppercase}
.bz svg.i{width:16px;height:16px}
.bz .w1,.bz .w2{opacity:.25}
body.buz .bz{color:var(--am);border-color:rgba(255,182,72,.6);background:rgba(255,182,72,.1);box-shadow:0 0 16px -4px var(--am)}
body.buz .bz .w1{animation:wv .6s infinite}
body.buz .bz .w2{animation:wv .6s .15s infinite}
@keyframes wv{0%{opacity:.15}50%{opacity:1}100%{opacity:.15}}
.leg{display:flex;gap:6px;margin-top:12px}
.leg span{flex:1;font-size:9.5px;font-weight:800;letter-spacing:.12em;padding:6px 4px;border-radius:8px;text-align:center;border:1px solid}
.leg .a{color:var(--rd);border-color:rgba(255,59,92,.35);background:rgba(255,59,92,.06)}
.leg .b{color:var(--am);border-color:rgba(255,182,72,.35);background:rgba(255,182,72,.06)}
.leg .c{color:var(--ok);border-color:rgba(53,255,160,.3);background:rgba(53,255,160,.05)}

/* ---------- motors ---------- */
.gg{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:10px}
.gauge{position:relative;text-align:center;padding:6px 6px 10px;border-radius:16px;background:radial-gradient(90% 70% at 50% 40%,rgba(37,230,255,.06),transparent 70%);border:1px solid rgba(255,255,255,.04)}
.gauge svg{display:block;width:100%;max-width:170px;margin:0 auto;overflow:visible}
.gauge .tk{stroke:rgba(160,200,255,.28);stroke-width:1.4}
.gv{position:absolute;left:0;right:0;top:34%;font-family:var(--fm);pointer-events:none}
.gv b{display:block;font-size:24px;font-weight:700;color:#fff;line-height:1}
.gv small{font-size:9px;letter-spacing:.3em;color:var(--mut);font-weight:700}
.gn{font-size:11px;font-weight:800;letter-spacing:.24em;color:#cfe3ff;margin-top:-4px;display:flex;align-items:center;justify-content:center;gap:6px}
.gn i{width:8px;height:8px;border-radius:2px;background:var(--gc);box-shadow:0 0 10px var(--gc)}
.pw{position:relative;height:6px;margin:9px 8px 4px;border-radius:6px;background:rgba(255,255,255,.07);overflow:hidden}
.pw::after{content:"";position:absolute;left:50%;top:0;bottom:0;width:1px;background:rgba(255,255,255,.35)}
.pw i{position:absolute;top:0;bottom:0;left:50%;width:0;border-radius:6px;background:var(--gc);box-shadow:0 0 10px var(--gc)}
.pv{font-family:var(--fm);font-size:10.5px;color:var(--mut)}
.pv b{color:#dbe9ff;font-weight:600}
.mrow{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:10px}
.kv{padding:8px 6px;border-radius:12px;background:rgba(255,255,255,.03);border:1px solid rgba(255,255,255,.05);text-align:center}
.kv span{display:block;font-size:9px;letter-spacing:.2em;color:var(--mut);font-weight:800}
.kv b{display:block;font-family:var(--fm);font-size:15px;color:#e6f2ff;margin-top:3px;font-weight:600}

/* ---------- trace ---------- */
.trace{display:flex;flex-direction:column}
.trace canvas{display:block;width:100%;height:132px;flex:1 1 132px;min-height:132px;margin-top:10px}
.tl{display:flex;gap:10px;white-space:nowrap;font-size:10px;letter-spacing:.14em;color:var(--mut);font-weight:700;text-transform:uppercase}
.tl i{display:inline-block;width:14px;height:3px;border-radius:3px;margin-right:5px;vertical-align:middle}

/* ---------- stats ---------- */
.stats{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;padding:10px}
.st{display:flex;flex-direction:column;align-items:center;gap:4px;padding:8px 2px;border-radius:12px;background:rgba(255,255,255,.025)}
.st svg.i{color:var(--vi);width:17px;height:17px;filter:drop-shadow(0 0 6px rgba(143,91,255,.7))}
.st span{font-size:8.5px;letter-spacing:.2em;color:var(--mut);font-weight:800}
.st b{font-family:var(--fm);font-size:12.5px;color:#e6f2ff;font-weight:600;white-space:nowrap}
.foot{grid-column:1/-1;text-align:center;font-size:10px;letter-spacing:.24em;color:var(--dim);font-weight:700;padding:4px 0 2px}

/* ---------- dock ---------- */
.dock{position:fixed;left:0;right:0;bottom:0;z-index:20;padding:10px 12px calc(12px + env(safe-area-inset-bottom));
  background:linear-gradient(rgba(3,5,11,0),rgba(3,5,11,.92) 28%);pointer-events:none}
.dk{pointer-events:auto;max-width:700px;margin:0 auto;display:grid;grid-template-columns:1fr 1fr 1fr 78px;gap:8px;align-items:end;
  padding:8px;border-radius:24px;background:linear-gradient(160deg,rgba(26,40,72,.9),rgba(8,12,24,.94));border:1px solid rgba(130,200,255,.2);
  box-shadow:0 -10px 40px -10px rgba(37,230,255,.25),0 20px 40px rgba(0,0,0,.6),inset 0 1px 0 rgba(255,255,255,.08)}
.mb{position:relative;height:72px;border-radius:17px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:3px;
  color:#93a7c6;background:linear-gradient(180deg,#1a2843,#0c1426);border:1px solid rgba(130,190,255,.16);
  box-shadow:0 5px 0 #040812,0 10px 18px -6px rgba(0,0,0,.8),inset 0 1px 0 rgba(255,255,255,.09);transform:translateY(-4px);transition:transform .1s,box-shadow .1s,color .2s,border-color .2s;overflow:hidden}
.mb:active{transform:translateY(1px);box-shadow:0 0 0 #040812,0 4px 10px -4px rgba(0,0,0,.8),inset 0 1px 0 rgba(255,255,255,.06)}
.mb svg.i{width:26px;height:26px;stroke-width:1.9;transition:filter .2s}
.mb .t{font-size:12px;font-weight:800;letter-spacing:.18em}
.mb .d{font-size:8.5px;letter-spacing:.2em;color:var(--dim);font-weight:700}
.mb[data-m="0"]{--mc:var(--ok)}.mb[data-m="1"]{--mc:var(--cy)}.mb[data-m="2"]{--mc:var(--pk)}
.mb::before{content:"";position:absolute;inset:-60%;background:conic-gradient(from 0deg,transparent 0 70%,var(--mc) 85%,transparent 100%);opacity:0;animation:spin 2.4s linear infinite}
.mb::after{content:"";position:absolute;inset:2px;border-radius:15px;background:linear-gradient(180deg,#1b2a47,#0c1426)}
.mb>*{position:relative;z-index:1}
.mb.on{color:#fff;border-color:transparent;box-shadow:0 5px 0 #040812,0 0 26px -4px var(--mc),inset 0 1px 0 rgba(255,255,255,.1)}
.mb.on::before{opacity:1}
.mb.on::after{background:linear-gradient(180deg,#1d3150,#0b1324)}
.mb.on::after{background:linear-gradient(180deg,color-mix(in srgb,var(--mc) 22%,#14213a),#0b1324)}
.mb.on svg.i{color:var(--mc);filter:drop-shadow(0 0 8px var(--mc))}
.mb.on .d{color:var(--mc)}
.mb.pend::before{opacity:.6;animation-duration:.7s}
@keyframes spin{to{transform:rotate(360deg)}}
.es{position:relative;width:78px;height:78px;border-radius:50%;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:1px;color:#fff;
  background:radial-gradient(circle at 50% 35%,#ff7f92,#ff2d4f 45%,#b0102d 80%);
  box-shadow:0 6px 0 #6b0618,0 0 0 4px rgba(255,59,92,.18),0 0 34px -4px rgba(255,59,92,.9),inset 0 2px 2px rgba(255,255,255,.45),inset 0 -6px 10px rgba(80,0,20,.5);
  transform:translateY(-5px);transition:transform .1s,box-shadow .1s}
.es:active{transform:translateY(1px);box-shadow:0 0 0 #6b0618,0 0 0 4px rgba(255,59,92,.25),0 0 20px -4px rgba(255,59,92,.9),inset 0 2px 2px rgba(255,255,255,.4),inset 0 -6px 10px rgba(80,0,20,.5)}
.es svg.i{width:30px;height:30px;stroke-width:2.2}
.es span{font-size:10px;font-weight:900;letter-spacing:.2em}
.es::after{content:"";position:absolute;inset:-7px;border-radius:50%;border:2px solid rgba(255,59,92,.55);animation:ring 2s infinite}
@keyframes ring{0%{transform:scale(.9);opacity:.9}100%{transform:scale(1.25);opacity:0}}
body[data-st="3"] .es{filter:saturate(.35) brightness(.75)}
body[data-st="3"] .es::after{animation:none;opacity:0}

/* ---------- lockout overlay ---------- */
.lock{position:fixed;inset:0;z-index:40;display:none;align-items:center;justify-content:center;padding:24px;
  background:radial-gradient(70% 55% at 50% 45%,rgba(120,0,24,.82),rgba(12,0,4,.94));-webkit-backdrop-filter:blur(4px);backdrop-filter:blur(4px)}
.lock.show{display:flex;animation:fin .3s ease}
@keyframes fin{from{opacity:0}}
.lock::before,.lock::after{content:"";position:absolute;left:0;right:0;height:26px;opacity:.85;
  background:repeating-linear-gradient(-45deg,#ff3b5c 0 18px,#14030a 18px 36px);background-size:51px 26px;animation:haz 1s linear infinite}
.lock::before{top:0}.lock::after{bottom:0}
@keyframes haz{to{background-position:51px 0}}
.lb{position:relative;width:100%;max-width:380px;text-align:center;padding:30px 22px 22px;border-radius:26px;
  background:linear-gradient(160deg,rgba(60,6,20,.92),rgba(18,2,8,.95));border:1px solid rgba(255,59,92,.6);box-shadow:0 0 60px -10px rgba(255,59,92,.9),inset 0 0 40px -10px rgba(255,59,92,.6)}
.lic{width:84px;height:84px;margin:0 auto 6px;color:var(--rd);filter:drop-shadow(0 0 16px var(--rd));animation:pulse 1.1s infinite}
.lic svg{width:100%;height:100%;fill:none;stroke:currentColor;stroke-width:1.6;stroke-linecap:round;stroke-linejoin:round}
@keyframes pulse{50%{transform:scale(1.07);filter:drop-shadow(0 0 28px var(--rd))}}
.lt{font-size:26px;font-weight:900;letter-spacing:.16em;color:#fff;text-shadow:0 0 18px var(--rd)}
.ls{font-size:14px;color:#ffc4cf;margin-top:6px;font-weight:600;letter-spacing:.04em}
.ld{font-family:var(--fm);font-size:54px;font-weight:700;color:#fff;margin:14px 0 4px;text-shadow:0 0 24px var(--rd)}
.ld small{font-size:20px;color:#ff9fb0;margin-left:4px}
.lh{font-size:12px;color:#ff9fb0;letter-spacing:.06em;margin-bottom:18px}
.lb .rst{display:flex;width:100%}
.gh{margin-top:12px;font-size:11px;font-weight:800;letter-spacing:.22em;color:#ff9fb0;padding:10px;width:100%;border-radius:12px;border:1px solid rgba(255,159,176,.3)}

/* ---------- toast / offline ---------- */
.toast{position:fixed;left:50%;top:calc(14px + env(safe-area-inset-top));z-index:60;transform:translate(-50%,-140%);transition:transform .35s cubic-bezier(.2,.9,.2,1.2);
  padding:11px 16px;border-radius:14px;font-size:13px;font-weight:700;letter-spacing:.06em;white-space:nowrap;max-width:92vw;overflow:hidden;text-overflow:ellipsis;
  background:rgba(10,18,34,.94);border:1px solid rgba(37,230,255,.45);box-shadow:0 10px 30px rgba(0,0,0,.6),0 0 24px -6px var(--cy);color:#dff8ff}
.toast.show{transform:translate(-50%,0)}
.toast.err{border-color:rgba(255,59,92,.6);box-shadow:0 10px 30px rgba(0,0,0,.6),0 0 24px -6px var(--rd);color:#ffd5dc}
body.off .card{filter:saturate(.35) brightness(.8);transition:filter .5s}
.ch{white-space:nowrap}.ch em{overflow:hidden;text-overflow:ellipsis}
@media(max-width:370px){.hint{display:none}.tag{letter-spacing:.22em}.name{font-size:21px}.big{font-size:40px}.dist{font-size:38px}.dk{grid-template-columns:1fr 1fr 1fr 70px;gap:6px}.es{width:70px;height:70px}.mb .t{font-size:11px;letter-spacing:.12em}}
@media(prefers-reduced-motion:reduce){*{animation-duration:.01s!important;animation-iteration-count:1!important}}
</style>
</head>
<body class="wait" data-st="-1" data-md="-1" data-zone="0">
<svg width="0" height="0" style="position:absolute" aria-hidden="true">
<defs>
<linearGradient id="gLogo" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#25e6ff"/><stop offset="1" stop-color="#ff3fa4"/></linearGradient>
<linearGradient id="gF" gradientUnits="userSpaceOnUse" x1="0" y1="120" x2="120" y2="0"><stop offset="0" stop-color="#3f7cff"/><stop offset="1" stop-color="#25e6ff"/></linearGradient>
<linearGradient id="gR" gradientUnits="userSpaceOnUse" x1="120" y1="120" x2="0" y2="0"><stop offset="0" stop-color="#8f5bff"/><stop offset="1" stop-color="#ff3fa4"/></linearGradient>
<radialGradient id="gFan" cx="160" cy="182" r="170" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#25e6ff" stop-opacity=".22"/><stop offset="1" stop-color="#25e6ff" stop-opacity=".02"/></radialGradient>
<linearGradient id="gSw" x1="0" y1="1" x2="0" y2="0"><stop offset="0" stop-color="#25e6ff" stop-opacity=".0"/><stop offset="1" stop-color="#25e6ff" stop-opacity=".55"/></linearGradient>
<filter id="glow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="3" result="b"/><feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
<symbol id="i-logo" viewBox="0 0 48 48"><path class="hx" d="M24 3 42 13.5v21L24 45 6 34.5v-21Z" fill="rgba(10,20,40,.6)" stroke="url(#gLogo)" stroke-width="2.4" stroke-linejoin="round"/><path class="xx" d="M16.5 12.5 31.5 30.5M31.5 12.5 16.5 30.5" fill="none" stroke="url(#gLogo)" stroke-width="3.6" stroke-linecap="round"/><circle cx="24" cy="36.5" r="3.6" fill="#03050b" stroke="#25e6ff" stroke-width="2"/><circle cx="24" cy="21.5" r="2.3" fill="#fff"/></symbol>
<symbol id="i-stable" viewBox="0 0 24 24"><rect x="9" y="2.8" width="6" height="11" rx="2"/><path d="M12 6v3"/><circle cx="12" cy="17.2" r="3"/><path d="M2.5 8.5h3.4M18.1 8.5h3.4M3.5 12h2M18.5 12h2M3.5 21.2h17"/></symbol>
<symbol id="i-fwd" viewBox="0 0 24 24"><path d="M5.5 12.5 12 6l6.5 6.5"/><path d="M5.5 19 12 12.5l6.5 6.5" opacity=".5"/><path d="M12 2.5v1"/></symbol>
<symbol id="i-rev" viewBox="0 0 24 24"><path d="M5.5 11.5 12 18l6.5-6.5"/><path d="M5.5 5 12 11.5 18.5 5" opacity=".5"/><path d="M12 20.5v1"/></symbol>
<symbol id="i-stop" viewBox="0 0 24 24"><path d="M8.2 2.6h7.6l5.6 5.6v7.6l-5.6 5.6H8.2l-5.6-5.6V8.2Z"/><path d="M12 7v5"/><path d="M8.7 9.3a4.7 4.7 0 1 0 6.6 0"/></symbol>
<symbol id="i-reset" viewBox="0 0 24 24"><path d="M20.2 12a8.2 8.2 0 1 1-2.4-5.8"/><path d="M20.2 3.6v4.6h-4.6"/><circle cx="12" cy="12" r="2.2"/></symbol>
<symbol id="i-sonar" viewBox="0 0 24 24"><circle cx="4.5" cy="12" r="2"/><path d="M9 8.6a4.8 4.8 0 0 1 0 6.8M12.6 5.6a9 9 0 0 1 0 12.8M16.2 2.8a13 13 0 0 1 0 18.4"/></symbol>
<symbol id="i-buzz" viewBox="0 0 24 24"><path d="M3.5 9.4h3.6L12 5.3v13.4l-4.9-4.1H3.5Z"/><path class="w1" d="M15.4 9.2a4 4 0 0 1 0 5.6"/><path class="w2" d="M18.3 6.4a8 8 0 0 1 0 11.2"/></symbol>
<symbol id="i-motor" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><circle cx="12" cy="12" r="5.2"/><circle cx="12" cy="12" r="1.4"/><path d="M12 3v3.8M12 17.2V21M3 12h3.8M17.2 12H21M5.6 5.6l2.7 2.7M15.7 15.7l2.7 2.7M18.4 5.6l-2.7 2.7M8.3 15.7l-2.7 2.7"/></symbol>
<symbol id="i-cube" viewBox="0 0 24 24"><path d="M12 2.6 20.2 7.3v9.4L12 21.4l-8.2-4.7V7.3Z"/><path d="M3.8 7.3 12 12l8.2-4.7M12 12v9.4"/></symbol>
<symbol id="i-wave" viewBox="0 0 24 24"><path d="M2.5 12.5h3.2l2.6-6.5 4.1 12.5 3-9 2.1 3h4"/></symbol>
<symbol id="i-cpu" viewBox="0 0 24 24"><rect x="6" y="6" width="12" height="12" rx="2"/><rect x="9.6" y="9.6" width="4.8" height="4.8" rx="1"/><path d="M9 2.6v3M15 2.6v3M9 18.4v3M15 18.4v3M2.6 9h3M2.6 15h3M18.4 9h3M18.4 15h3"/></symbol>
<symbol id="i-clock" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3.4 2.1"/></symbol>
<symbol id="i-route" viewBox="0 0 24 24"><circle cx="5.5" cy="18.5" r="2.2"/><circle cx="18.5" cy="5.5" r="2.2"/><path d="M7.6 17.2c6-1 2.5-7.2 8.8-10.4" stroke-dasharray="2.2 2.6"/></symbol>
<symbol id="i-phone" viewBox="0 0 24 24"><rect x="6.5" y="2.5" width="11" height="19" rx="2.6"/><path d="M10.5 18.5h3"/><path d="M9.4 9.3a4 4 0 0 1 5.2 0M10.8 11.4a1.8 1.8 0 0 1 2.4 0"/></symbol>
<symbol id="i-wifi" viewBox="0 0 24 24"><path d="M2.5 9a14 14 0 0 1 19 0M5.8 12.6a9 9 0 0 1 12.4 0M9 16.1a4.4 4.4 0 0 1 6 0"/><path d="M12 19.4v.1"/></symbol>
<symbol id="i-lock" viewBox="0 0 24 24"><path d="M12 2.5 20 6v5.4c0 5-3.4 8.6-8 10.1-4.6-1.5-8-5.1-8-10.1V6Z"/><rect x="8.4" y="10.6" width="7.2" height="5.8" rx="1.3"/><path d="M9.9 10.6V9a2.1 2.1 0 0 1 4.2 0v1.6"/></symbol>
<symbol id="i-tilt" viewBox="0 0 24 24"><path d="M3 20.5h18"/><path d="M6.5 20.5 16 5"/><path d="M11.8 20.5a5.6 5.6 0 0 0-1.8-4.1"/><circle cx="16.6" cy="4.2" r="1.4"/></symbol>
</defs>
</svg>

<div class="bg" aria-hidden="true"><div class="aur"></div><div class="stars"></div><div class="floor"><div class="plane"></div></div><div class="scan"></div></div>

<main class="wrap">
  <header class="top">
    <svg class="logo" viewBox="0 0 48 48"><use href="#i-logo"/></svg>
    <div class="wm"><div class="name">BALANCE<span>X</span></div><div class="tag">ROBOT CONTROL DECK</div></div>
    <div class="link" id="link"><i></i><span id="linkT">LINKING</span><b id="rtt"></b></div>
  </header>

  <section class="card status" style="--i:1">
    <div class="chip stc"><i></i><span id="stT">CONNECTING</span></div>
    <div class="smsg" id="stM">Waiting for telemetry from BalanceX…</div>
    <div class="chip mdc" id="mdC"><svg class="i"><use id="mdI" href="#i-stable"/></svg><span id="mdT">—</span></div>
    <div class="prog"><i id="prog"></i></div>
    <button class="rst" data-c="reset"><svg class="i"><use href="#i-reset"/></svg>RESET &amp; RECALIBRATE</button>
  </section>

  <section class="card hero" style="--i:2">
    <div class="ch"><svg class="i"><use href="#i-cube"/></svg>Holo-Twin<em>live attitude</em><span class="sp"></span><span class="hint">DRAG TO ORBIT</span></div>
    <div class="stage" id="stage"><div class="world" id="world"></div></div>
    <div class="hud">
      <div class="l"><svg class="i"><use href="#i-tilt"/></svg>TILT ANGLE</div>
      <div class="big"><span id="tiltV">0.00</span><small>°</small></div>
      <div class="s"><span>RATE <b id="rateV">0.0</b></span><span>TARGET <b id="tgtV">0.00</b></span></div>
    </div>
    <div class="hud2"><span class="tagx" id="tgArm">DISARMED</span><span class="tagx" id="tgSon">SONAR</span></div>
    <svg class="att" id="att" viewBox="0 0 320 74" preserveAspectRatio="xMidYMid meet"></svg>
  </section>

  <section class="card sonar" style="--i:3">
    <div class="ch"><svg class="i"><use href="#i-sonar"/></svg>Sonar<em>path scan</em><span class="sp"></span><span class="bz"><svg class="i"><use href="#i-buzz"/></svg><span id="bzT">BUZZER OFF</span></span></div>
    <div class="rbox"><svg class="radar" id="radar" viewBox="0 0 320 196"></svg></div>
    <div class="srow">
      <div><div class="dist"><span id="distV">--</span><small id="distU">m</small></div><div class="dsub" id="distS">OBJECT DISTANCE</div></div>
      <div class="zone" id="zone">SCANNING</div>
    </div>
    <div class="leg"><span class="a">&lt; 1 m · LOCKOUT</span><span class="b">&lt; 2 m · BUZZER</span><span class="c">&gt; 2 m · CLEAR</span></div>
  </section>

  <section class="card motors" style="--i:4">
    <div class="ch"><svg class="i"><use href="#i-motor"/></svg>Drive<em>encoder speed</em></div>
    <div class="gg" id="gg"></div>
    <div class="mrow">
      <div class="kv"><span>TARGET</span><b id="kT">0.00</b></div>
      <div class="kv"><span>CRUISE</span><b id="kC">0.00</b></div>
      <div class="kv"><span>SYNC Δ</span><b id="kS">0.00</b></div>
    </div>
  </section>

  <section class="card trace" style="--i:5">
    <div class="ch"><svg class="i"><use href="#i-wave"/></svg>Telemetry<span class="sp"></span>
      <span class="tl"><span><i style="background:#25e6ff;box-shadow:0 0 8px #25e6ff"></i>tilt</span><span><i style="background:#ff3fa4"></i>target</span><span><i style="background:#35ffa0"></i>rpm</span></span></div>
    <canvas id="chart"></canvas>
  </section>

  <section class="card stats" style="--i:6">
    <div class="st"><svg class="i"><use href="#i-cpu"/></svg><span>LOOP</span><b id="sW">--</b></div>
    <div class="st"><svg class="i"><use href="#i-clock"/></svg><span>UPTIME</span><b id="sU">--</b></div>
    <div class="st"><svg class="i"><use href="#i-route"/></svg><span>TRAVEL</span><b id="sP">--</b></div>
    <div class="st"><svg class="i"><use href="#i-phone"/></svg><span>CLIENTS</span><b id="sC">--</b></div>
    <div class="foot" id="foot">BALANCEX · ESP32 · 192.168.4.1</div>
  </section>
</main>

<nav class="dock"><div class="dk">
  <button class="mb" data-c="stable" data-m="0" aria-label="Stable mode"><svg class="i"><use href="#i-stable"/></svg><span class="t">STABLE</span><span class="d">HOLD</span></button>
  <button class="mb" data-c="forward" data-m="1" aria-label="Forward mode"><svg class="i"><use href="#i-fwd"/></svg><span class="t">FORWARD</span><span class="d">SLOW AHEAD</span></button>
  <button class="mb" data-c="reverse" data-m="2" aria-label="Reverse mode"><svg class="i"><use href="#i-rev"/></svg><span class="t">REVERSE</span><span class="d">SLOW BACK</span></button>
  <button class="es" data-c="estop" aria-label="Emergency stop"><svg class="i"><use href="#i-stop"/></svg><span>STOP</span></button>
</div></nav>

<div class="lock" id="lock" role="alertdialog" aria-label="Obstacle lockout"><div class="lb">
  <div class="lic"><svg viewBox="0 0 24 24"><use href="#i-lock"/></svg></div>
  <div class="lt">LOCKOUT</div>
  <div class="ls">Obstacle inside 1 m · motors disarmed</div>
  <div class="ld"><span id="lkD">--</span><small>m</small></div>
  <div class="lh" id="lkH">Clear the path, stand the robot upright, then reset.</div>
  <button class="rst" data-c="reset"><svg class="i"><use href="#i-reset"/></svg>RESET &amp; RECALIBRATE</button>
  <button class="gh" id="lkX">VIEW DASHBOARD</button>
</div></div>

<div class="toast" id="toast"></div>

<script>
(()=>{
'use strict';
const $=id=>document.getElementById(id), B=document.body, NS='http://www.w3.org/2000/svg';
const clamp=(v,a,b)=>Math.min(b,Math.max(a,v)), sgn=(v,d)=>(v>=0?'+':'−')+Math.abs(v).toFixed(d);
const el=(t,c,p)=>{const e=document.createElement(t);if(c)e.className=c;if(p)p.appendChild(e);return e};
const sv=(t,a,p)=>{const e=document.createElementNS(NS,t);for(const k in a)e.setAttribute(k,a[k]);if(p)p.appendChild(e);return e};
let touched=false;addEventListener('pointerdown',()=>touched=true,{once:true,capture:true});
const vib=p=>{try{touched&&navigator.vibrate&&navigator.vibrate(p)}catch(e){}};
const STN=['CALIBRATING','READY','BALANCING','STOPPED'];
const MODES=[['STABLE','#i-stable','holding position','var(--ok)'],['FORWARD','#i-fwd','rolling forward slowly','var(--cy)'],['REVERSE','#i-rev','rolling backward slowly','var(--pk)']];
const FAULT={1:['IMU ERROR','MPU6050 not responding. Check SDA 21 / SCL 22, then reset.'],2:['CALIBRATION MOVED','Robot moved while calibrating. Hold it still, then reset.'],
 3:['TIMING ERROR','Control loop stalled. Reset to recalibrate.'],4:['FALLEN','Tilt passed 25°. Stand the robot upright, then reset.'],
 5:['EXCESS RATE','Rotation rate limit exceeded. Reset to recalibrate.'],6:['OVERSPEED','Wheel speed limit exceeded. Reset to recalibrate.'],
 7:['ENCODER / STALL','Wheels did not follow the command. Check encoders, then reset.'],8:['MOTOR WATCHDOG','Motor command deadline missed. Reset to re-arm.'],
 9:['EMERGENCY STOP','Stopped by operator. Reset to recalibrate and re-arm.'],10:['OBSTACLE LOCKOUT','Object inside 1 m, motors disarmed. Clear the path, then reset.']};
const SAY={stable:'STABLE · holding position',forward:'FORWARD · slow ahead',reverse:'REVERSE · slow back',estop:'EMERGENCY STOP · motors off',reset:'RECALIBRATING · hold upright & still'};
const RMAX=50, MPX=40; // gauge full scale (rpm), radar px per metre

/* ---------- 3D holo-twin ---------- */
const world=$('world');
function box(p,w,h,d,x,y,z,c){
  const b=el('div',c,p);b.style.transform=`translate3d(${x}px,${y}px,${z}px)`;
  [['f',w,h,`translateZ(${d/2}px)`],['k',w,h,`rotateY(180deg) translateZ(${d/2}px)`],['r',d,h,`rotateY(90deg) translateZ(${w/2}px)`],
   ['l',d,h,`rotateY(-90deg) translateZ(${w/2}px)`],['t',w,d,`rotateX(90deg) translateZ(${h/2}px)`],['b',w,d,`rotateX(-90deg) translateZ(${h/2}px)`]]
  .forEach(([n,fw,fh,tr])=>{const f=el('i','fc '+n,b);f.style.cssText=`width:${fw}px;height:${fh}px;margin:${-fh/2}px 0 0 ${-fw/2}px;transform:${tr}`});
  return b;
}
const L=d=>58+d*42; // scene distance scale (px for metres)
const ground=el('div','ground',world);ground.style.transform='translateY(42px) rotateX(90deg)';
(()=>{let s=`<svg viewBox="-320 -320 640 640"><defs><radialGradient id="gG"><stop offset="0" stop-color="#25e6ff" stop-opacity=".22"/><stop offset=".55" stop-color="#25e6ff" stop-opacity=".05"/><stop offset="1" stop-color="#25e6ff" stop-opacity="0"/></radialGradient>
<pattern id="gP" width="32" height="32" patternUnits="userSpaceOnUse" x="-16" y="-16"><path d="M32 0H0V32" fill="none" stroke="#25e6ff" stroke-opacity=".22" stroke-width="1"/></pattern>
<radialGradient id="gM"><stop offset=".25" stop-color="#fff"/><stop offset="1" stop-color="#fff" stop-opacity="0"/></radialGradient><mask id="gK"><circle r="320" fill="url(#gM)"/></mask>
<radialGradient id="gS"><stop offset="0" stop-color="#000" stop-opacity=".75"/><stop offset="1" stop-color="#000" stop-opacity="0"/></radialGradient></defs>
<circle r="320" fill="url(#gG)"/><rect x="-320" y="-320" width="640" height="640" fill="url(#gP)" mask="url(#gK)"/>`;
const pt=(r,a)=>[(r*Math.sin(a*Math.PI/180)).toFixed(1),(r*Math.cos(a*Math.PI/180)).toFixed(1)];
const wedge=(r0,r1,A)=>{const[a,b]=pt(r0,-A),[c,d]=pt(r1,-A),[e,f]=pt(r1,A),[g,h]=pt(r0,A);return `M${a} ${b}L${c} ${d}A${r1} ${r1} 0 0 0 ${e} ${f}L${g} ${h}A${r0} ${r0} 0 0 1 ${a} ${b}Z`};
s+=`<path d="${wedge(30,L(1),32)}" fill="#ff3b5c" fill-opacity=".22" stroke="#ff3b5c" stroke-opacity=".7"/>`;
s+=`<path d="${wedge(L(1),L(2),32)}" fill="#ffb648" fill-opacity=".12" stroke="#ffb648" stroke-opacity=".55"/>`;
[[1,'#ff3b5c'],[2,'#ffb648'],[3,'#25e6ff'],[4,'#25e6ff']].forEach(([m,c])=>{const[x,y]=pt(L(m),-36);s+=`<text x="${x}" y="${y}" fill="${c}" font-size="13" font-weight="700" font-family="monospace" text-anchor="end" transform="rotate(0)">${m}m</text>`});
s+=`<ellipse rx="78" ry="40" fill="url(#gS)"/></svg>`;ground.innerHTML=s})();
const rob=el('div','',world), body=el('div','',rob);
box(body,84,20,22,0,0,0,'mot');
box(body,104,6,62,0,-14,0,'pl');
box(body,40,14,40,-24,-24,-4,'drv');
[[-46,-26],[46,-26],[-46,26],[46,26]].forEach(([x,z])=>box(body,4,96,4,x,-62,z,'post'));
box(body,104,6,62,0,-62,0,'pl');
box(body,54,4,26,12,-67,4,'pcb');
box(body,16,3,14,-30,-66.5,10,'imu');
box(body,12,9,12,34,-69.5,-17,'buzz');
box(body,104,6,62,0,-110,0,'pl');
box(body,80,18,34,0,-122,0,'bat');
box(body,58,24,3,0,-38,32.5,'son');
[-15,15].forEach(x=>{const e=el('div','eye',body);e.style.transform=`translate3d(${x}px,-38px,34px)`;
  for(let k=0;k<4;k++){const d=el('i',k==3?'c':'',e);d.style.transform=`translateZ(${k*2.4}px)`}});
const beam=el('div','beam',body);beam.style.transform='translate3d(0,-38px,36px)';
const bh=el('i','',beam), bv=el('i','',beam);bh.style.transform='rotateX(90deg)';bv.style.transform='rotateZ(90deg) rotateX(90deg)';
function wheel(x,c){const w=el('div','wheel '+c,rob);for(let k=0;k<7;k++){const d=el('i',k==0||k==6?'cap':'rg',w);d.style.transform=`translateZ(${-11+k*22/6}px)`}return w}
const wL=wheel(64,'wL'), wR=wheel(-64,'wR');
const wall=el('div','wall',world);wall.innerHTML='<b id="wallD">--</b><s>OBJECT</s>';

let yaw=50,pitch=-16,drag=null,lastInput=0;
const stage=$('stage');
stage.addEventListener('pointerdown',e=>{drag={x:e.clientX,y:e.clientY,yaw,pitch};lastInput=performance.now()});
addEventListener('pointermove',e=>{if(!drag)return;yaw=drag.yaw+(e.clientX-drag.x)*.45;pitch=clamp(drag.pitch-(e.clientY-drag.y)*.25,-42,-4);lastInput=performance.now()});
addEventListener('pointerup',()=>drag=null);addEventListener('pointercancel',()=>drag=null);
stage.addEventListener('dblclick',()=>{yaw=50;pitch=-16});

/* ---------- attitude arc ---------- */
(()=>{const a=$('att'),cx=160,cy=330,R=300,ang=t=>t*.9*Math.PI/180,P=(t,r)=>[cx+r*Math.sin(ang(t)),cy-r*Math.cos(ang(t))];
 const seg=(t0,t1,col,w,op)=>{const[x0,y0]=P(t0,R),[x1,y1]=P(t1,R);sv('path',{d:`M${x0} ${y0}A${R} ${R} 0 0 1 ${x1} ${y1}`,fill:'none',stroke:col,'stroke-width':w,'stroke-opacity':op,'stroke-linecap':'round'},a)};
 seg(-30,30,'#7f93b0',2,.35);seg(-5,5,'#35ffa0',4,.9);seg(20,25,'#ffb648',4,.8);seg(-25,-20,'#ffb648',4,.8);seg(25,30,'#ff3b5c',4,.9);seg(-30,-25,'#ff3b5c',4,.9);
 for(let t=-30;t<=30;t+=5){const mj=t%10==0,[x0,y0]=P(t,R+3),[x1,y1]=P(t,R+(mj?13:8));sv('line',{x1:x0,y1:y0,x2:x1,y2:y1,stroke:'#9fb6d6','stroke-opacity':mj?.8:.4,'stroke-width':mj?1.6:1},a);
  if(mj){const[tx,ty]=P(t,R+24);sv('text',{x:tx,y:ty+3,'text-anchor':'middle'},a).textContent=(t>0?'+':'')+t}}
 const g=sv('g',{id:'attN'},a);sv('path',{d:`M${cx} ${cy-R-1}l-7 -12h14z`,fill:'#fff',filter:'url(#glow)'},g);
 const k=sv('g',{id:'attT'},a);sv('path',{d:`M${cx} ${cy-R+4}l4 6-4 6-4-6z`,fill:'#ff3fa4'},k);
})();

/* ---------- radar ---------- */
const OX=160,OY=182;
const rp=(r,a)=>[OX+r*Math.sin(a*Math.PI/180),OY-r*Math.cos(a*Math.PI/180)];
const rwedge=(r0,r1,A)=>{const[a,b]=rp(r0,-A),[c,d]=rp(r1,-A),[e,f]=rp(r1,A),[g,h]=rp(r0,A);return `M${a} ${b}L${c} ${d}A${r1} ${r1} 0 0 1 ${e} ${f}L${g} ${h}`+(r0?`A${r0} ${r0} 0 0 0 ${a} ${b}`:'')+'Z'};
const rarc=(r,A)=>{const[a,b]=rp(r,-A),[c,d]=rp(r,A);return `M${a} ${b}A${r} ${r} 0 0 1 ${c} ${d}`};
(()=>{const s=$('radar');
 sv('path',{d:rwedge(0,166,50),fill:'url(#gFan)',stroke:'#25e6ff','stroke-opacity':.25},s);
 sv('path',{d:rwedge(0,MPX,50),fill:'#ff3b5c','fill-opacity':.16},s);
 sv('path',{d:rwedge(MPX,2*MPX,50),fill:'#ffb648','fill-opacity':.09},s);
 [[1,'#ff3b5c',.8],[2,'#ffb648',.7],[3,'#25e6ff',.3],[4,'#25e6ff',.3]].forEach(([m,c,o])=>{sv('path',{d:rarc(m*MPX,50),fill:'none',stroke:c,'stroke-opacity':o,'stroke-dasharray':m>2?'3 4':'0'},s);
  const[x,y]=rp(m*MPX,55);sv('text',{x:x+2,y:y+3},s).textContent=m+'m'});
 [-50,-25,0,25,50].forEach(a=>{const[x,y]=rp(166,a);sv('line',{x1:OX,y1:OY,x2:x,y2:y,stroke:'#25e6ff','stroke-opacity':.13},s)});
 sv('path',{d:rwedge(0,166,15),fill:'#25e6ff','fill-opacity':.05,stroke:'#25e6ff','stroke-opacity':.25,'stroke-dasharray':'2 3'},s);
 sv('path',{class:'sw',d:rwedge(0,166,5),fill:'url(#gSw)'},s);
 sv('circle',{id:'rip',class:'rp',cx:OX,cy:OY-80,r:8,fill:'none',stroke:'#fff','stroke-width':2},s);
 sv('path',{id:'obs',class:'obs',d:rarc(80,14),fill:'none',stroke:'#25e6ff','stroke-width':5,'stroke-linecap':'round',filter:'url(#glow)',opacity:0},s);
 sv('path',{d:`M${OX-9} ${OY+8}h18l-3-14h-12z`,fill:'#0b1426',stroke:'#25e6ff','stroke-width':1.4},s);
 sv('circle',{cx:OX,cy:OY-2,r:2.4,fill:'#25e6ff',filter:'url(#glow)'},s);
})();

/* ---------- motor gauges ---------- */
const GA=(cx,cy,r,a0,a1)=>{const p=a=>[cx+r*Math.cos(a*Math.PI/180),cy+r*Math.sin(a*Math.PI/180)],[x0,y0]=p(a0),[x1,y1]=p(a1);
 return `M${x0.toFixed(2)} ${y0.toFixed(2)}A${r} ${r} 0 ${Math.abs(a1-a0)>180?1:0} ${a1>a0?1:0} ${x1.toFixed(2)} ${y1.toFixed(2)}`};
const gauges=[['LEFT','var(--cy)'],['RIGHT','var(--pk)']].map(([n,c])=>{
 const g=el('div','gauge',$('gg'));g.style.setProperty('--gc',c);
 const s=sv('svg',{viewBox:'0 0 120 108'},g);
 sv('path',{d:GA(60,60,46,135,405),fill:'none',stroke:'rgba(255,255,255,.07)','stroke-width':9,'stroke-linecap':'round'},s);
 for(let k=0;k<=12;k++){const a=(135+k*22.5)*Math.PI/180,r0=k%3?37:34;sv('line',{class:'tk',x1:60+r0*Math.cos(a),y1:60+r0*Math.sin(a),x2:60+39*Math.cos(a),y2:60+39*Math.sin(a)},s)}
 const arc=sv('path',{fill:'none','stroke-width':9,'stroke-linecap':'round',filter:'url(#glow)'},s);
 sv('circle',{cx:60,cy:14,r:2,fill:'#fff','fill-opacity':.7},s);
 const t1=sv('text',{x:22,y:104,'font-size':8,fill:'#7f93b0','font-family':'monospace'},s);t1.textContent='REV';
 const t2=sv('text',{x:98,y:104,'font-size':8,fill:'#7f93b0','font-family':'monospace','text-anchor':'end'},s);t2.textContent='FWD';
 const v=el('div','gv',g);v.innerHTML='<b>0.0</b><small>RPM</small>';
 const nm=el('div','gn',g);nm.innerHTML='<i></i>'+n;
 const pw=el('div','pw',g),pi=el('i','',pw),pv=el('div','pv',g);pv.innerHTML='PWM <b>0</b>';
 return {arc,val:v.firstChild,pi,pv:pv.lastChild};
});
function setGauge(G,rpm,pwm){
 const f=clamp(rpm/RMAX,-1,1);
 if(Math.abs(f)<.004)G.arc.setAttribute('d','');else{G.arc.setAttribute('d',GA(60,60,46,270,270+f*135));G.arc.setAttribute('stroke',f>0?'url(#gF)':'url(#gR)')}
 const p=clamp(pwm/180,-1,1);G.pi.style.left=(p<0?50+p*50:50)+'%';G.pi.style.width=Math.abs(p*50)+'%';
}

/* ---------- telemetry ---------- */
let D=null,lastOk=0,rtt=0,pend=-1,pendT=0,lockHide=false,lastFt=-1,lastSt=-1,lastDist=-1;
const V={tilt:0,rate:0,tgt:0,lr:0,rr:0,lp:0,rp:0,dist:400};
const trace=[];
let spinL=0,spinR=0;

async function poll(){
 const t0=performance.now();let ok=false;
 const ac=new AbortController(),to=setTimeout(()=>ac.abort(),1500);
 try{const r=await fetch('/api/state',{cache:'no-store',signal:ac.signal});if(r.ok){const j=await r.json();rtt=performance.now()-t0;lastOk=performance.now();ok=true;apply(j)}}catch(e){}
 clearTimeout(to);
 setTimeout(poll,document.hidden?1000:ok?Math.max(15,90-(performance.now()-t0)):350);
}

function apply(j){
 D=j;const now=performance.now();
 B.classList.remove('wait');
 B.dataset.st=j.st;B.dataset.md=j.md;B.dataset.zone=j.son?j.zone:0;
 B.classList.toggle('buz',!!j.buz);B.classList.toggle('snoff',!j.son);
 trace.push({t:now,a:j.tilt,g:j.tgt,v:(j.lr+j.rr)/2});
 while(trace.length&&trace[0].t<now-12600)trace.shift();
 if(pend>=0&&(j.md===pend||now-pendT>2000))pend=-1;
 document.querySelectorAll('.mb').forEach(b=>{const m=+b.dataset.m;b.classList.toggle('on',m===j.md);b.classList.toggle('pend',m===pend&&m!==j.md)});
 const M=MODES[j.md]||MODES[0];$('mdT').textContent=M[0];$('mdI').setAttribute('href',M[1]);$('mdC').style.setProperty('--mc',M[3]);
 let st=STN[j.st]||'--',msg='',pr=0;
 if(j.st===0){msg=`Hold upright &amp; perfectly still · <b>${Math.max(0,(5000-j.cal)/1000).toFixed(1)} s</b>`;pr=j.cal/50}
 else if(j.st===1){pr=j.arm;msg=j.blk?'<b>Arming blocked</b> · object inside 1 m, clear the path':'Calibrated · hold upright &amp; still to <b>auto-arm</b>'}
 else if(j.st===2){msg=`Motors armed · <b>${M[0]}</b> ${M[2]}`}
 else{const f=FAULT[j.ft]||['STOPPED','Motors off.'];st=j.ft===10?'LOCKOUT':'STOPPED';msg=`<b>${f[0]}</b> · ${f[1]}`}
 $('stT').textContent=st;$('stM').innerHTML=msg;$('prog').style.width=clamp(pr,0,100)+'%';
 const ta=$('tgArm');ta.textContent=j.st===2?'ARMED':j.st===1&&j.blk?'ARM BLOCKED':'DISARMED';ta.className='tagx'+(j.st===2?' on':j.st===1&&j.blk?' bd':'');
 const ts=$('tgSon');ts.textContent=j.son?'SONAR LIVE':'SONAR OFFLINE';ts.className='tagx'+(j.son?(j.zone===2?' bd':j.zone===1?' wr':' on'):'');
 // sonar
 const clear=!j.son||j.dist>=399.5;
 if(!j.son){$('distV').textContent='--';$('distU').textContent='';$('distS').textContent='NO ECHO FROM SENSOR';$('zone').textContent='SENSOR OFFLINE'}
 else if(clear){$('distV').textContent='>4.0';$('distU').textContent='m';$('distS').textContent='NOTHING IN RANGE';$('zone').textContent='PATH CLEAR'}
 else{$('distV').textContent=(j.dist/100).toFixed(2);$('distU').textContent='m';$('distS').textContent=Math.round(j.dist)+' CM AHEAD';$('zone').textContent=j.zone===2?'LOCKOUT ZONE':j.zone===1?'OBJECT AHEAD':'PATH CLEAR'}
 $('bzT').textContent=j.buz?'BUZZER ON':'BUZZER OFF';
 const ob=$('obs');
 if(clear)ob.setAttribute('opacity',0);else{ob.setAttribute('opacity',1);ob.setAttribute('d',rarc(clamp(j.dist/100,0.05,4.1)*MPX,14));ob.setAttribute('stroke',j.zone===2?'#ff3b5c':j.zone===1?'#ffb648':'#25e6ff');
  if(Math.abs(j.dist-lastDist)>0.4){const r=$('rip');r.setAttribute('cy',OY-clamp(j.dist/100,0.05,4.1)*MPX);r.classList.remove('go');void r.getBBox();r.classList.add('go')}}
 lastDist=j.dist;
 $('wallD').textContent=clear?'':(j.dist/100).toFixed(2)+' m';
 // drive
 $('kT').textContent=sgn(j.trpm,2);$('kC').textContent=sgn(j.crpm,2);$('kS').textContent=Math.abs(j.lr-j.rr).toFixed(2);
 // stats
 $('sW').textContent=j.work+' µs';const u=Math.floor(j.up/1000);$('sU').textContent=`${String(Math.floor(u/3600)).padStart(2,'0')}:${String(Math.floor(u/60)%60).padStart(2,'0')}:${String(u%60).padStart(2,'0')}`;
 $('sP').textContent=sgn(j.pos,2)+' rev';$('sC').textContent=j.cli;
 // lockout overlay
 if(j.ft!==lastFt){if(j.ft===10){lockHide=false;vib([120,60,120,60,260])}else if(j.ft===4||j.ft===9)vib(80);lastFt=j.ft}
 if(j.st!==lastSt){if(j.st===2&&lastSt>=0)vib(30);lastSt=j.st}
 $('lkD').textContent=j.son&&!clear?(j.dist/100).toFixed(2):'--';
 $('lkH').textContent=j.son&&j.dist<100?'Object still inside 1 m. Clear the path, then reset.':'Path is clear. Stand the robot upright, then reset.';
 $('lock').classList.toggle('show',j.st===3&&j.ft===10&&!lockHide);
}

/* ---------- commands ---------- */
let tt=0;
function toast(m,err){const t=$('toast');t.textContent=m;t.className='toast show'+(err?' err':'');clearTimeout(tt);tt=setTimeout(()=>t.className='toast'+(err?' err':''),2200)}
async function send(c,btn){
 vib(c==='estop'?[60,40,60]:14);
 if(btn&&btn.dataset.m!=null){pend=+btn.dataset.m;pendT=performance.now();btn.classList.add('pend')}
 try{const r=await fetch('/api/cmd?c='+c,{method:'POST',cache:'no-store'});if(!r.ok)throw 0;toast(SAY[c]||'OK');if(c==='reset')lockHide=true}
 catch(e){toast('Command not delivered · check WiFi link',1);pend=-1;document.querySelectorAll('.mb.pend').forEach(b=>b.classList.remove('pend'))}
}
document.querySelectorAll('[data-c]').forEach(b=>b.addEventListener('click',()=>send(b.dataset.c,b)));
$('lkX').addEventListener('click',()=>{lockHide=true;$('lock').classList.remove('show')});

/* ---------- chart ---------- */
const cv=$('chart'),cx=cv.getContext('2d');
function chart(now){
 const w=cv.clientWidth,h=cv.clientHeight,dp=Math.min(2,devicePixelRatio||1);if(!w||!h)return;
 if(cv.width!==Math.round(w*dp)||cv.height!==Math.round(h*dp)){cv.width=Math.round(w*dp);cv.height=Math.round(h*dp)}
 cx.setTransform(dp,0,0,dp,0,0);cx.clearRect(0,0,w,h);
 let m=4;for(const p of trace)m=Math.max(m,Math.abs(p.a),Math.abs(p.g));m=Math.min(30,Math.ceil(m/2+.5)*2);
 const pad=26,X=t=>pad+(w-pad)*(1-(now-t)/12000),Y=v=>h/2-v/m*(h/2-8),Yr=v=>h/2-clamp(v/RMAX,-1,1)*(h/2-8);
 cx.font='9px ui-monospace,monospace';cx.textBaseline='middle';
 [-1,-.5,0,.5,1].forEach(f=>{const y=Y(f*m);cx.strokeStyle=f?'rgba(120,190,255,.09)':'rgba(120,190,255,.28)';cx.lineWidth=1;cx.beginPath();cx.moveTo(pad,y);cx.lineTo(w,y);cx.stroke();
  cx.fillStyle='rgba(127,147,176,.85)';cx.fillText((f>0?'+':'')+(f*m).toFixed(0)+'°',0,y)});
 for(let s=0;s<=12;s+=2){const x=pad+(w-pad)*(1-s/12);cx.strokeStyle='rgba(120,190,255,.06)';cx.beginPath();cx.moveTo(x,4);cx.lineTo(x,h-4);cx.stroke()}
 const line=(key,col,glow,dash,Yf)=>{if(trace.length<2)return;cx.save();cx.beginPath();trace.forEach((p,i)=>{const x=X(p.t),y=Yf(p[key]);i?cx.lineTo(x,y):cx.moveTo(x,y)});
  cx.strokeStyle=col;cx.lineWidth=glow?2.2:1.5;cx.setLineDash(dash||[]);if(glow){cx.shadowColor=col;cx.shadowBlur=10}cx.stroke();cx.restore()};
 line('v','rgba(53,255,160,.75)',0,[],Yr);line('g','#ff3fa4',0,[5,4],Y);line('a','#25e6ff',1,null,Y);
 if(trace.length){const p=trace[trace.length-1];cx.fillStyle='#fff';cx.shadowColor='#25e6ff';cx.shadowBlur=12;cx.beginPath();cx.arc(X(p.t),Y(p.a),3,0,7);cx.fill();cx.shadowBlur=0}
}

/* ---------- render loop ---------- */
let tp=performance.now(),tTxt=0,tCh=0;
function frame(t){
 const dt=Math.min(.1,(t-tp)/1000);tp=t;const k=1-Math.exp(-dt*14);
 if(D){V.tilt+=(D.tilt-V.tilt)*k;V.rate+=(D.rate-V.rate)*k;V.tgt+=(D.tgt-V.tgt)*k;V.lr+=(D.lr-V.lr)*k;V.rr+=(D.rr-V.rr)*k;V.lp+=(D.lp-V.lp)*k;V.rp+=(D.rp-V.rp)*k;
  V.dist+=(((D.son&&D.dist<399.5)?D.dist:420)-V.dist)*k}
 const off=lastOk>0?t-lastOk>1600:t>5000;B.classList.toggle('off',off);if(off)B.classList.remove('wait');
 if(off){$('linkT').textContent='OFFLINE';$('rtt').textContent='';$('stT').textContent='NO SIGNAL';$('stM').innerHTML='Reconnect to WiFi <b>BalanceX</b> · dashboard resumes automatically'}
 else if(lastOk){$('linkT').textContent='LIVE'}
 // 3D
 if(!drag&&t-lastInput>4000)yaw+=(50+Math.sin(t/2600)*8-yaw)*Math.min(1,dt*1.5);
 const sc=clamp(Math.min(stage.clientWidth/370,stage.clientHeight/330),.85,1.45);
 world.style.transform=`scale3d(${sc},${sc},${sc}) rotateX(${pitch}deg) rotateY(${yaw}deg)`;
 const tl=clamp(V.tilt,-90,90);body.style.transform=`rotateX(${-tl}deg)`;
 spinL-=V.lr*6*dt;spinR-=V.rr*6*dt;spinL%=360;spinR%=360;
 wL.style.transform=`translate3d(64px,0,0) rotateY(90deg) rotateZ(${spinL}deg)`;wR.style.transform=`translate3d(-64px,0,0) rotateY(90deg) rotateZ(${spinR}deg)`;
 const has=D&&D.son&&D.dist<399.5,Lp=L(clamp(V.dist/100,0,4.3));
 beam.style.setProperty('--L',(has?Lp-36:L(4)-36)+'px');beam.style.setProperty('--bw',((has?Lp:L(4))*.42)+'px');beam.style.opacity=D&&D.son?1:.15;
 wall.style.transform=`translate3d(0,-44px,${Lp}px)`;wall.style.opacity=has?1:0;
 // attitude needle
 const an=$('attN'),at=$('attT');an.setAttribute('transform',`rotate(${clamp(V.tilt,-33,33)*.9} 160 330)`);at.setAttribute('transform',`rotate(${clamp(V.tgt,-33,33)*.9} 160 330)`);
 // gauges
 setGauge(gauges[0],V.lr,V.lp);setGauge(gauges[1],V.rr,V.rp);
 if(t-tTxt>70){tTxt=t;
  $('tiltV').textContent=sgn(V.tilt,2);$('rateV').textContent=sgn(V.rate,1)+'°/s';$('tgtV').textContent=sgn(V.tgt,2)+'°';
  gauges[0].val.textContent=sgn(V.lr,1);gauges[1].val.textContent=sgn(V.rr,1);
  if(D){gauges[0].pv.textContent=D.lp;gauges[1].pv.textContent=D.rp;$('rtt').textContent=off?'':Math.round(rtt)+'ms'}
 }
 if(t-tCh>33){tCh=t;chart(t)}
 requestAnimationFrame(frame);
}
$('foot').textContent='BALANCEX · ESP32 · '+(location.host||'192.168.4.1');
requestAnimationFrame(frame);poll();
})();
</script>
</body>
</html>
)BXUI";
