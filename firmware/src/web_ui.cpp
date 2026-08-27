/**
 * ArmPilot - Gomulu web arayuzu (tek dosya, harici bagimlilik yok)
 *
 * Arayuz robotu iki 2B goruntu ile temsil eder:
 *   - YAN GORUNUS : omuz / dirsek / bilek / kiskac  (kolun kendi duzlemi)
 *   - UST GORUNUS : taban donusu
 *
 * Kol dogrudan surukleyerek hareket ettirilir. Uc noktayi surukleyince ters
 * kinematik (IK) calisir, tek eklem tutamacini surukleyince ileri kinematik.
 * Geometri, limitler ve aci eslemesi ESP32'den (config.h) gelir.
 */
#include "web_ui.h"

const char INDEX_HTML[] PROGMEM = R"PAGE(
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>ArmPilot</title>
<!-- Tema, stil dosyasindan ONCE secilir: aksi halde acik temayi kullanan
     kullanici her yenilemede bir kare koyu ekran gorur. -->
<script>(function(){var s=null;try{s=localStorage.getItem("ap.theme");}catch(e){}
document.documentElement.setAttribute("data-theme",
  (s==="light"||s==="dark") ? s
  : (matchMedia("(prefers-color-scheme:light)").matches ? "light" : "dark"));})();</script>
<style>
/* ---------- tema ----------
   Butun renkler degisken. KOYU tema :root'ta, ACIK tema yalnizca degiskenleri
   ezer -> kural gövdelerinde tek bir sabit renk kalmaz, iki tema tek yerden
   yonetilir. Aktif tema <html data-theme> ile secilir (sayfa basindaki kucuk
   betik, yanip sonme olmasin diye onu CSS'ten once yazar). */
:root{
  /* yuzeyler */
  --bg:#080b10; --glow:#12202c; --pan1:#121a25; --pan2:#0e151e;
  --card:#0c141d; --head:#0c131b; --bar:#0b1219; --track:#070d14;
  --soft:#1b2836; --line:#1d2a3a; --line2:#2a3d52; --shadow:rgba(0,0,0,.6);
  /* yazi */
  --txt:#dbe6f3; --txt2:#a8bccf; --dim:#7b8da0; --dim2:#54687c;
  /* vurgu */
  --acc:#22d3ee; --acc2:#0ea5b7; --accd:#4d7f8c; --fill1:#0f5766;
  --a0:rgba(34,211,238,.05); --a1:rgba(34,211,238,.14);
  --a2:rgba(34,211,238,.34); --a3:rgba(34,211,238,.5);
  --onacc:#04141a; --onok:#04140a; --mark:#f1f5f9;
  /* durum renkleri */
  --ok:#22c55e; --warn:#f59e0b; --bad:#ef4444; --okdim:#7fae94;
  --okbd:#1c5433; --okbg:#0d1c14; --wbd:#5c4310; --wbg:#1c1509;
  --bbd:#5d1f1f; --bbg:#1c0d0d; --abd:#155e6b; --abg:#08191d;
  --ntbg:#17120a; --nttx:#f5d193;
  /* dugmeler */
  --btn:#16212d; --btnh:#1b2937; --btnbd:#4a6a86; --tgb:#101923; --tgbd:#3d5771;
  --pri1:#17c8e0; --pri2:#0c8fa4; --pribd:#4de6ff;
  --dng:#2a1414; --dngbd:#7a3030; --dngtx:#ffb0b0; --thumbbd:#06202a;
  /* 2B gorunusler */
  --sgrid:#16222f; --sgnd:#2b3d4f; --shat:#1b2836; --senv:#1b2c3a;
  --sbase:#243244; --sbasel:#3c536c; --stop:#2b3b4f; --stopl:#435c78;
  --lo:#4b6883; --li:#8fb2cf; --disc:#2f4257; --discl:#5b7d9e;
  --limp1:#3a4655; --limp2:#63707f; --limpd:#2a3340; --limpl:#4a5768;
  --lbl:#63798f; --dial:#101a24; --tick:#2a3d52; --tickm:#42596f;
  --mono:ui-monospace,"Cascadia Mono",Consolas,monospace;
}
:root[data-theme=light]{
  --bg:#eef2f7; --glow:#dbe9f3; --pan1:#ffffff; --pan2:#f6f9fc;
  --card:#f2f6fa; --head:#eaf0f6; --bar:#eef3f8; --track:#e2e8f0;
  --soft:#dde5ee; --line:#d8e0ea; --line2:#bcc8d6; --shadow:rgba(15,23,42,.16);
  --txt:#132030; --txt2:#3b4b5c; --dim:#5d6f82; --dim2:#7e8f9f;
  --acc:#0e7490; --acc2:#0891b2; --accd:#6f9aa8; --fill1:#67d0e2;
  --a0:rgba(14,116,144,.06); --a1:rgba(14,116,144,.12);
  --a2:rgba(14,116,144,.26); --a3:rgba(14,116,144,.42);
  --onacc:#ffffff; --onok:#ffffff; --mark:#0f172a;
  --ok:#15803d; --warn:#a35a06; --bad:#dc2626; --okdim:#4b8b63;
  --okbd:#a8d5b8; --okbg:#e9f7ee; --wbd:#e6c88f; --wbg:#fdf4e3;
  --bbd:#f0b6b6; --bbg:#fdeceb; --abd:#a5d6e2; --abg:#e6f6fa;
  --ntbg:#fdf4e3; --nttx:#8a5a10;
  --btn:#ffffff; --btnh:#e9f0f7; --btnbd:#8fa8bd; --tgb:#ffffff; --tgbd:#8fa8bd;
  --pri1:#22b8d4; --pri2:#0e7490; --pribd:#0e7490;
  --dng:#fdecec; --dngbd:#e8a5a5; --dngtx:#b02525; --thumbbd:#ffffff;
  --sgrid:#e4ebf3; --sgnd:#8ba0b4; --shat:#c9d5e1; --senv:#c8d7e4;
  --sbase:#c9d6e3; --sbasel:#8ba0b6; --stop:#dae4ee; --stopl:#9db0c3;
  --lo:#8ba6bd; --li:#31506c; --disc:#c4d3e0; --discl:#6d8aa4;
  --limp1:#c3cbd4; --limp2:#8d99a6; --limpd:#d6dce3; --limpl:#a9b4bf;
  --lbl:#6b7c8d; --dial:#ffffff; --tick:#bcc8d6; --tickm:#8496a8;
}
*{box-sizing:border-box}
html,body{height:100%}
/* Klavyeyle gezinenler icin gorunur odak. Fare tiklamalarinda cikmaz. */
:focus-visible{outline:2px solid var(--acc);outline-offset:2px;border-radius:4px}
@media(prefers-reduced-motion:reduce){
  *{transition:none!important;animation:none!important}
}
body{
  margin:0;background:
    radial-gradient(1200px 700px at 20% -10%,var(--glow) 0%,transparent 60%),
    var(--bg);
  color:var(--txt);font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;
  -webkit-user-select:none;user-select:none;
}
.app{max-width:1400px;margin:0 auto;padding:14px 16px 22px}

/* ---------- header ---------- */
header{display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin-bottom:12px}
.brand{font-size:1.15rem;letter-spacing:.16em;font-weight:600;display:flex;align-items:center;gap:9px}
.brand i{color:var(--acc);font-style:normal;font-size:1.3rem;line-height:1}
.brand b{color:var(--acc);font-weight:800}
.brand small{color:var(--dim);letter-spacing:.06em;font-size:.62rem;margin-left:4px}
.spacer{flex:1}
.badge{
  font:600 .68rem/1 var(--mono);letter-spacing:.12em;padding:7px 11px;border-radius:999px;
  border:1px solid var(--line2);color:var(--dim);background:var(--card);white-space:nowrap;
}
.badge.ok{color:var(--ok);border-color:var(--okbd);background:var(--okbg)}
.badge.warn{color:var(--warn);border-color:var(--wbd);background:var(--wbg)}
.badge.bad{color:var(--bad);border-color:var(--bbd);background:var(--bbg)}
.badge.acc{color:var(--acc);border-color:var(--abd);background:var(--abg)}
.estop{
  font:800 .74rem/1 var(--mono);letter-spacing:.14em;color:#fff;cursor:pointer;
  background:linear-gradient(#e0322f,#a81b1b);border:1px solid #ff6b6b;
  padding:10px 16px;border-radius:9px;box-shadow:0 0 0 0 rgba(239,68,68,.5);
  transition:box-shadow .15s,transform .1s;
}
.estop:hover{box-shadow:0 0 16px rgba(239,68,68,.45)}
.estop:active{transform:translateY(1px)}

/* ---------- notice ---------- */
.notice{
  display:none;gap:11px;align-items:flex-start;padding:11px 14px;border-radius:10px;
  border:1px solid var(--wbd);background:var(--ntbg);color:var(--nttx);
  font-size:.82rem;line-height:1.5;margin-bottom:12px;
}
.notice.show{display:flex}
.notice b{color:var(--warn)}
.notice s{color:var(--acc);text-decoration:none;font-weight:700}

/* ---------- layout ---------- */
.grid{display:grid;grid-template-columns:minmax(0,1.55fr) minmax(300px,1fr);gap:12px;align-items:start}
@media(max-width:900px){.grid{grid-template-columns:1fr}}
.col{display:flex;flex-direction:column;gap:12px}
#top{max-height:252px}
#grip{max-height:168px}
.panel{background:linear-gradient(180deg,var(--pan1),var(--pan2));border:1px solid var(--line);border-radius:14px;overflow:hidden}
.ph{display:flex;align-items:center;gap:9px;padding:10px 13px;border-bottom:1px solid var(--line);background:var(--head)}
.ph h2{margin:0;font-size:.72rem;letter-spacing:.16em;text-transform:uppercase;color:var(--txt);font-weight:700}
.ph .hint{margin-left:auto;font-size:.66rem;color:var(--dim);letter-spacing:.03em}
.ph .dot{width:7px;height:7px;border-radius:50%;background:var(--acc);box-shadow:0 0 8px var(--acc)}
.pb{padding:6px}
svg{display:block;width:100%;height:auto;touch-action:none}

/* ---------- svg styling ---------- */
.grid-l{stroke:var(--sgrid);stroke-width:1}
.ground{stroke:var(--sgnd);stroke-width:2}
.hatch{stroke:var(--shat);stroke-width:1.5}
.base-body{fill:var(--sbase);stroke:var(--sbasel);stroke-width:1.5}
.base-top{fill:var(--stop);stroke:var(--stopl);stroke-width:1.5}
.envelope{fill:none;stroke:var(--senv);stroke-width:1.2;stroke-dasharray:5 7}
.link{stroke-linecap:round;fill:none}
.link-o{stroke:var(--lo)}
.link-i{stroke:var(--li)}
.disc{fill:var(--disc);stroke:var(--discl);stroke-width:2}
.disc-c{fill:var(--li)}
.finger{stroke:var(--li);stroke-linecap:round;fill:none}
.limp .link-o{stroke:var(--limp1)}
.limp .link-i{stroke:var(--limp2)}
.limp .disc{fill:var(--limpd);stroke:var(--limpl)}
.limp .disc-c{fill:var(--limp2)}
.limp .finger{stroke:var(--limp2)}
.ghost{fill:none;stroke:var(--acc);stroke-width:2;stroke-dasharray:4 5;opacity:.55;stroke-linecap:round}
.ghost-d{fill:none;stroke:var(--acc);stroke-width:1.5;opacity:.5}
/* kalibrasyon modundaki "esleme kolu" - gercek kolun gozle eslenen duruşu */
.match line{stroke:var(--warn);stroke-linecap:round;fill:none}
.match .match-d{fill:var(--wbg);stroke:var(--warn);stroke-width:2}
/* cal modunda model kol geri plana dussun ki esleme kolu okunakli kalsin */
.dim{opacity:.42}
.handle{fill:var(--a1);stroke:var(--acc);stroke-width:2;cursor:grab}
.handle:hover{fill:var(--a2)}
.handle.tip{fill:var(--a2);stroke-width:2.5}
.handle.act{fill:var(--a3);cursor:grabbing}
.lbl{fill:var(--lbl);font:600 11px var(--mono);letter-spacing:.06em}
.lbl-a{fill:var(--acc);font:700 12px var(--mono)}
.fan{fill:var(--a0);stroke:var(--senv);stroke-width:1}
.dial{fill:var(--dial);stroke:var(--tick);stroke-width:1.5}
.tick{stroke:var(--tick);stroke-width:1.5}
.tick.maj{stroke:var(--tickm)}
.beam{stroke:var(--acc);stroke-width:3;stroke-linecap:round}
.beam-g{stroke:var(--lo);stroke-width:6;stroke-linecap:round}

/* ---------- toolbar ---------- */
.tools{display:flex;flex-wrap:wrap;gap:7px;padding:9px 11px;border-top:1px solid var(--line);background:var(--bar)}
.tg{
  font:600 .68rem/1 var(--mono);letter-spacing:.07em;color:var(--dim);cursor:pointer;
  background:var(--tgb);border:1px solid var(--line2);padding:8px 11px;border-radius:8px;
}
.tg:hover:not(:disabled){border-color:var(--tgbd);color:var(--txt)}
.tg:disabled{opacity:.35;cursor:not-allowed}
.tg.on{color:var(--onacc);background:var(--acc);border-color:var(--acc);font-weight:800}
.tools .rd{margin-left:auto;font:600 .7rem var(--mono);color:var(--dim);align-self:center}
.tools .rd b{color:var(--acc);font-weight:700}

/* ---------- joint strip ---------- */
.jstrip{display:grid;grid-template-columns:repeat(5,1fr);gap:10px;padding:11px}
@media(max-width:760px){.jstrip{grid-template-columns:repeat(2,1fr)}}
.jcard{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:9px 11px 11px}
.jn{font:600 .64rem var(--mono);letter-spacing:.14em;color:var(--dim);text-transform:uppercase}
.jv{font:700 1.5rem/1.1 var(--mono);color:var(--acc);margin:3px 0 7px}
.jv i{font-style:normal;font-size:.9rem;color:var(--accd)}
.jbar{position:relative;height:8px;border-radius:5px;background:var(--track);border:1px solid var(--line);overflow:visible}
.jbar .fillc{position:absolute;top:0;bottom:0;left:0;background:linear-gradient(90deg,var(--fill1),var(--acc));border-radius:5px}
.jbar .ghostm{position:absolute;top:-4px;width:2px;height:14px;background:var(--mark);opacity:.9;border-radius:1px}
.jt{margin-top:6px;font:600 .63rem var(--mono);letter-spacing:.08em;color:var(--dim2)}
.jt b{color:var(--txt2);font-weight:700}

/* ---------- calibration ---------- */
.calgrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:9px 22px;padding:10px 14px 4px}
.calrow{display:grid;grid-template-columns:64px 1fr 44px 54px;gap:11px;align-items:center}
.calrow span{font:600 .66rem var(--mono);letter-spacing:.09em;color:var(--dim);text-transform:uppercase}
.calrow b{font:700 .76rem var(--mono);color:var(--acc);text-align:right}
/* 1. noktadan bu yana o eksenin ne kadar oynadigi. Kullanicinin "ayni pozu
   iki kez kaydettim" tuzagina dusmemesi icin kaydiricinin TAM yaninda. */
.calrow b.delta{font-size:.7rem;color:var(--dim2)}
.calrow b.delta.ok{color:var(--ok)}

/* yan gorunusteki uc farkli kolun ne oldugunu soyleyen aciklama seridi */
.legend{display:flex;flex-wrap:wrap;gap:6px 18px;padding:9px 13px;
        border-top:1px solid var(--line);background:var(--bar)}
.legend[hidden]{display:none}
.legend span{display:flex;align-items:center;gap:7px;
             font:600 .66rem var(--mono);letter-spacing:.04em;color:var(--dim)}
.legend i{width:16px;height:3px;border-radius:2px;flex:none}
.legend .sw-model{background:var(--li)}
.legend .sw-match{background:var(--warn)}
.legend .note{color:var(--dim2);font-weight:500;margin-left:auto;text-align:right}

/* --- adim seridi: kullanici akisin neresinde oldugunu bir bakista gorsun --- */
.steps{display:flex;list-style:none;margin:0;padding:12px 14px 2px;gap:6px;flex-wrap:wrap}
.step{display:flex;align-items:center;gap:8px;padding:7px 11px;border-radius:9px;
      border:1px solid var(--line);background:var(--card);flex:1;min-width:148px}
.step b{width:20px;height:20px;border-radius:50%;display:grid;place-items:center;flex:none;
        font:700 .68rem var(--mono);background:var(--soft);color:var(--dim)}
.step i{font-style:normal;font:600 .65rem var(--mono);letter-spacing:.07em;
        color:var(--dim);text-transform:uppercase}
.step.done b{background:var(--ok);color:var(--onok)}
.step.done i{color:var(--okdim)}
.step.act{border-color:var(--acc);background:var(--abg)}
.step.act b{background:var(--acc);color:var(--onacc)}
.step.act i{color:var(--txt)}

/* --- aktif adimin talimati: ekranda her an TEK bir "simdi sunu yap" --- */
.guide{margin:9px 14px 2px;padding:12px 14px;border-radius:10px;
       border:1px solid var(--line2);background:var(--card)}
.guide h3{margin:0 0 6px;font:800 .71rem var(--mono);letter-spacing:.14em;color:var(--acc)}
.guide p{margin:0;font-size:.81rem;line-height:1.62;color:var(--txt2)}
.guide p+p{margin-top:6px}
.guide b{color:var(--txt);font-weight:700}
.guide s{color:var(--warn);text-decoration:none;font-weight:700}
.chips{display:flex;gap:7px;flex-wrap:wrap;margin-top:10px}
.chip{font:700 .64rem var(--mono);letter-spacing:.06em;padding:5px 10px;border-radius:999px;
      border:1px solid var(--line2);color:var(--dim);background:var(--track)}
.chip.ok{color:var(--ok);border-color:var(--okbd);background:var(--okbg)}
.res{margin:10px 14px 0;padding:9px 13px;border-radius:9px;font:600 .74rem/1.5 var(--mono)}
.res.ok{color:var(--ok);border:1px solid var(--okbd);background:var(--okbg)}
.res.bad{color:var(--warn);border:1px solid var(--wbd);background:var(--wbg)}
.subhead{padding:14px 14px 0;font:700 .65rem var(--mono);letter-spacing:.14em;
         text-transform:uppercase;color:var(--dim)}
.subhead i{font-style:normal;font-weight:500;text-transform:none;letter-spacing:.02em;color:var(--dim2)}
.adv{margin-top:8px;border-top:1px solid var(--line)}
.adv summary{padding:10px 14px;cursor:pointer;list-style:none;
             font:700 .65rem var(--mono);letter-spacing:.12em;text-transform:uppercase;color:var(--dim)}
.adv summary::-webkit-details-marker{display:none}
.adv summary:before{content:"▸ ";color:var(--acc)}
.adv[open] summary:before{content:"▾ "}
.adv summary:hover{color:var(--txt)}
.adv .tools{border-top:none;background:transparent}

/* Kalibrasyonda surukleme YUKARIDAKI yan gorunuste yapilir, dugmeler ise
   panelde. Ikisi ayni ekrana sigmadigi icin eylem cubugunu alta sabitliyoruz. */
.calbar{
  position:fixed;left:0;right:0;bottom:0;z-index:50;display:flex;gap:9px;align-items:center;
  flex-wrap:wrap;padding:10px 14px;border-top:1px solid var(--acc2);background:var(--bar);
  box-shadow:0 -8px 26px var(--shadow);
}
.calbar[hidden]{display:none}          /* display:flex, hidden'i ezerdi */
.calstat{font:600 .68rem var(--mono);letter-spacing:.05em;color:var(--dim);margin-right:auto}
.calstat b{color:var(--acc);font-weight:700}
body.calon{padding-bottom:78px}

/* ---------- gripper ---------- */
.grip-wrap{padding:4px 12px 12px}

/* ---------- command bar ---------- */
.cmdbar{
  margin-top:12px;display:flex;flex-wrap:wrap;gap:9px;align-items:center;
  background:linear-gradient(180deg,var(--pan1),var(--pan2));border:1px solid var(--line);
  border-radius:14px;padding:11px 13px;
}
.btn{
  font:700 .72rem/1 var(--mono);letter-spacing:.1em;cursor:pointer;color:var(--txt);
  background:var(--btn);border:1px solid var(--line2);padding:11px 15px;border-radius:9px;
  transition:.13s;
}
.btn:hover:not(:disabled){border-color:var(--btnbd);background:var(--btnh)}
.btn:disabled{opacity:.35;cursor:not-allowed}
.btn.primary{background:linear-gradient(var(--pri1),var(--pri2));color:var(--onacc);border-color:var(--pribd);font-weight:800}
.btn.primary:hover:not(:disabled){box-shadow:0 0 14px var(--a2)}
.btn.danger{background:var(--dng);border-color:var(--dngbd);color:var(--dngtx)}
.speed{display:flex;align-items:center;gap:10px;margin-left:auto;min-width:250px;flex:1;max-width:400px}
.speed span{font:600 .66rem var(--mono);letter-spacing:.11em;color:var(--dim);white-space:nowrap}
.speed b{font:700 .74rem var(--mono);color:var(--acc);min-width:38px;text-align:right}
input[type=range]{
  -webkit-appearance:none;appearance:none;flex:1;height:5px;border-radius:3px;
  background:var(--soft);outline:none;cursor:pointer;
}
input[type=range]::-webkit-slider-thumb{
  -webkit-appearance:none;width:17px;height:17px;border-radius:50%;
  background:var(--acc);border:2px solid var(--thumbbd);cursor:pointer;
}
input[type=range]::-moz-range-thumb{
  width:15px;height:15px;border-radius:50%;background:var(--acc);border:2px solid var(--thumbbd);cursor:pointer;
}
footer{margin-top:10px;text-align:center;color:var(--dim2);font:500 .66rem var(--mono);letter-spacing:.08em}
.keys{margin-top:7px;text-align:center;color:var(--dim2);font:500 .64rem var(--mono);letter-spacing:.06em}
kbd{display:inline-block;padding:2px 6px;margin:0 1px;border-radius:5px;border:1px solid var(--line2);
    background:var(--tgb);color:var(--txt2);font:700 .62rem var(--mono)}
</style>

<div class="app">
  <header>
    <div class="brand"><i>&#9670;</i>ARM<b>PILOT</b><small id="fwv"></small></div>
    <div class="spacer"></div>
    <span class="badge" id="bRst" data-i18n-title="badge.rst.title">&#10227; --</span>
    <span class="badge" id="bMove" role="status" aria-live="polite">&#8226; DURGUN</span>
    <span class="badge" id="bState" role="status" aria-live="polite">--</span>
    <span class="badge" id="bLink" role="status" aria-live="polite">BAGLANIYOR</span>
    <button class="tg" id="btnLang">EN</button>
    <button class="tg" id="btnTheme">&#9788; ACIK</button>
    <button class="estop" id="btnEstop" data-i18n-title="estop.title">ACIL STOP</button>
  </header>

  <div class="notice" id="notice" role="status" aria-live="polite">
    <b>&#9888;</b>
    <div id="noticeTxt"></div>
  </div>

  <div class="grid">
    <!-- ================= YAN GORUNUS ================= -->
    <section class="panel">
      <div class="ph">
        <span class="dot"></span><h2 data-i18n="side.title">Kol Duzlemi &mdash; Yan Gorunus</h2>
        <span class="hint" id="sideHint">Eklemi veya ucu surukle</span>
      </div>
      <div class="pb"><svg id="side" preserveAspectRatio="xMidYMid meet"></svg></div>
      <div class="legend" id="sideLegend" hidden>
        <span data-i18n-html="legend.model"><i class="sw-model"></i>model kol &mdash; yazilimin sandigi duruş</span>
        <span data-i18n-html="legend.match"><i class="sw-match"></i>turuncu kol &mdash; senin esledigin gercek duruş</span>
        <span class="note" data-i18n-html="legend.note">Surukleyince TURUNCU kol oynar. Gercek kolu oynatmak icin
          asagidaki <b>kaydiricilar</b>.</span>
      </div>
      <div class="tools">
        <button class="tg on" id="tgLevel" data-i18n="tg.level">KISKAC YATAY</button>
        <button class="tg" id="tgElbow">DIRSEK: YUKARI</button>
        <button class="tg" id="tgGhost" data-i18n="tg.ghost">HEDEF IZI</button>
        <span class="rd"><span data-i18n="rd.tip">UC:</span> X <b id="rdX">0</b> Y <b id="rdY">0</b> mm</span>
      </div>
    </section>

    <div class="col">
      <!-- ================= UST GORUNUS ================= -->
      <section class="panel">
        <div class="ph"><span class="dot"></span><h2 data-i18n="top.title">Taban Donusu</h2><span class="hint" data-i18n="top.hint">Cevir</span></div>
        <div class="pb"><svg id="top" viewBox="0 0 260 175" preserveAspectRatio="xMidYMid meet"></svg></div>
      </section>

      <!-- ================= KISKAC ================= -->
      <section class="panel">
        <div class="ph"><span class="dot"></span><h2 data-i18n="grip.title">Kiskac</h2><span class="hint" id="gripHint">--</span></div>
        <div class="pb"><svg id="grip" viewBox="0 0 260 122" preserveAspectRatio="xMidYMid meet"></svg></div>
        <div class="grip-wrap"><input type="range" id="gripRange" min="0" max="180" step="1" value="60"
             data-i18n-aria="grip.aria" aria-label="Kiskac servo acisi"></div>
      </section>
    </div>
  </div>

  <!-- ================= EKSENLER ================= -->
  <section class="panel" style="margin-top:12px">
    <div class="ph"><span class="dot"></span><h2 data-i18n="joints.title">Eksenler</h2>
      <span class="hint" data-i18n-html="joints.hint">buyuk deger = anlik konum &nbsp;|&nbsp; beyaz cizgi = hedef</span></div>
    <div class="jstrip" id="joints"></div>
  </section>

  <!-- ================= KALIBRASYON ================= -->
  <section class="panel" id="calPanel" style="margin-top:12px">
    <div class="ph"><span class="dot"></span><h2 data-i18n="cal.title">Kalibrasyon</h2>
      <span class="hint" id="calSum">&mdash;</span>
      <button class="tg" id="calOpen" style="margin-left:12px">PANELI AC</button></div>

    <div id="calBody" style="display:none">
      <ol class="steps" id="calSteps">
        <li class="step" id="cs0"><b>1</b><i data-i18n="cal.step1">Kolu etkinlestir</i></li>
        <li class="step" id="cs1"><b>2</b><i data-i18n="cal.step2">1. pozu esle</i></li>
        <li class="step" id="cs2"><b>3</b><i data-i18n="cal.step3">2. pozu esle</i></li>
        <li class="step" id="cs3"><b>4</b><i data-i18n="cal.step4">Hesapla</i></li>
      </ol>

      <div id="calRes"></div>
      <div class="guide" id="calGuide" aria-live="polite"></div>

      <div class="subhead" data-i18n-html="cal.move.head">Gercek kolu oynat
        <i>&mdash; bu kaydiricilar servoyu dogrudan surer (turuncu kolu degil)</i></div>
      <div class="calgrid" id="calSliders"></div>

      <details class="adv">
        <summary data-i18n="cal.adv">Gelismis</summary>
        <div class="tools">
          <button class="tg" id="calCap" data-i18n="cal.cap" data-i18n-title="cal.cap.title">HIZLI: DIK YUKARI</button>
          <button class="tg" id="calD0" title="Omuz ekseninin yonunu ters cevir">OMUZ YON</button>
          <button class="tg" id="calD1" title="Dirsek ekseninin yonunu ters cevir">DIRSEK YON</button>
          <button class="tg" id="calD2" title="Bilek ekseninin yonunu ters cevir">BILEK YON</button>
          <button class="tg" id="calKin" data-i18n-title="cal.kin.title">KINEMATIK: SERI</button>
          <button class="tg" id="calGo" data-i18n="cal.gripOpen">KISKAC TAM ACIK</button>
          <button class="tg" id="calGc" data-i18n="cal.gripClose">KISKAC TAM KAPALI</button>
          <button class="tg" id="calRst" data-i18n="cal.reset">SIFIRLA</button>
        </div>
      </details>
    </div>
  </section>

  <div class="cmdbar">
    <button class="btn primary" id="btnEngage" data-i18n="cmd.engage" data-i18n-title="cmd.engage.title">ETKINLESTIR</button>
    <button class="btn" id="btnHome" data-i18n-title="cmd.home.title">HOME</button>
    <button class="btn" id="btnPark" data-i18n-title="cmd.park.title">PARK</button>
    <button class="btn" id="btnSave" data-i18n="cmd.save" data-i18n-title="cmd.save.title">POZU KAYDET</button>
    <button class="btn" id="btnCal" data-i18n-title="cmd.cal.title">KALIBRASYON</button>
    <button class="btn danger" id="btnRelease" data-i18n="cmd.release" data-i18n-title="cmd.release.title">SERBEST BIRAK</button>
    <div class="speed">
      <span data-i18n="cmd.speed">HIZ</span>
      <input type="range" id="spd" min="15" max="130" step="1" value="85" data-i18n-aria="cmd.speed.aria" aria-label="Hiz olcegi">
      <b id="spdV">0.85x</b>
    </div>
  </div>

  <footer id="foot">&mdash;</footer>
  <div class="keys" data-i18n-html="keys">
    <kbd>Space</kbd> acil stop &nbsp;·&nbsp; <kbd>H</kbd> home &nbsp;·&nbsp; <kbd>P</kbd> park
  </div>
</div>

<!-- Kalibrasyon eylem cubugu: mod acikken ekranin altina sabitlenir, boylece
     yukarida surukleme yaparken de her an erisilebilir kalir. -->
<div class="calbar" id="calActions" hidden>
  <span class="calstat" id="calStat">&mdash;</span>
  <button class="btn" id="calQuit" data-i18n="cal.quit">CIKIS</button>
  <button class="btn" id="calClr" data-i18n="cal.clear">ORNEKLERI SIL</button>
  <button class="btn primary" id="calPrimary">&mdash;</button>
</div>

<script>
"use strict";
const NS = "http://www.w3.org/2000/svg";
const D2R = Math.PI/180, R2D = 180/Math.PI;
const clamp = (v,a,b)=>v<a?a:(v>b?b:v);
const norm180 = a=>{while(a>180)a-=360;while(a<=-180)a+=360;return a;};
const el = (t,a)=>{const e=document.createElementNS(NS,t);for(const k in a)e.setAttribute(k,a[k]);return e;};

/* ===================== dil (TR / EN) =====================
   Tum kullanici metinleri tek sozlukte. Statik isaretleme data-i18n /
   data-i18n-html / data-i18n-title / data-i18n-aria nitelikleriyle etiketli;
   calisma aninda degisen metinler (rozetler, sihirbaz) dogrudan L() cagirir.
   Secim tarayicida kalir (ESP32'nin NVS'ini mesgul etmeye degmez). Hic secim
   yoksa tarayicinin dili belirler: tr* -> Turkce, digerleri -> Ingilizce.
   %1 yer tutucusu L(anahtar, deger) ile doldurulur. */
const STR = {
tr:{
"lang.switch":"Switch to English",
"badge.rst.title":"Son acilis sebebi / reset sayaci",
"badge.idle":"● DURGUN", "badge.moving":"▶ HAREKET", "badge.reset":"RESET x",
"link.connecting":"BAGLANIYOR", "link.on":"BAGLI", "link.off":"KOPUK",
"theme.light":"&#9788; ACIK", "theme.dark":"&#9789; KOYU",
"theme.toLight":"Acik temaya gec", "theme.toDark":"Koyu temaya gec",
"estop":"ACIL STOP", "estop.title":"Kisayol: Space", "resume":"DEVAM ET",
"state.0":"SERBEST", "state.1":"DEVREYE...", "state.2":"AKTIF", "state.3":"ACIL STOP",

"side.title":"Kol Duzlemi — Yan Gorunus",
"side.hint":"Eklemi veya ucu surukle",
"side.hint.cal":"TURUNCU kolu gercek kola benzet",
"legend.model":'<i class="sw-model"></i>model kol &mdash; yazilimin sandigi durus',
"legend.match":'<i class="sw-match"></i>turuncu kol &mdash; senin esledigin gercek durus',
"legend.note":"Surukleyince TURUNCU kol oynar. Gercek kolu oynatmak icin asagidaki <b>kaydiricilar</b>.",
"tg.level":"KISKAC YATAY", "tg.elbow.up":"DIRSEK: YUKARI", "tg.elbow.down":"DIRSEK: ASAGI",
"tg.ghost":"HEDEF IZI", "rd.tip":"UC:",
"top.title":"Taban Donusu", "top.hint":"Cevir",
"grip.title":"Kiskac", "grip.aria":"Kiskac servo acisi", "grip.openSuffix":"% ACIK",
"joints.title":"Eksenler", "joints.target":"HEDEF",
"joints.hint":"buyuk deger = anlik konum &nbsp;|&nbsp; beyaz cizgi = hedef",

"jn.0":"Taban", "jn.1":"Omuz", "jn.2":"Dirsek", "jn.3":"Bilek", "jn.4":"Kiskac",
"ax.0":"OMUZ", "ax.1":"DIRSEK", "ax.2":"BILEK",

"cal.title":"Kalibrasyon", "cal.open":"PANELI AC", "cal.close":"KAPAT",
"cal.step1":"Kolu etkinlestir", "cal.step2":"1. pozu esle",
"cal.step3":"2. pozu esle", "cal.step4":"Hesapla",
"cal.move.head":"Gercek kolu oynat <i>&mdash; bu kaydiricilar servoyu dogrudan surer (turuncu kolu degil)</i>",
"cal.adv":"Gelismis",
"cal.cap":"HIZLI: DIK YUKARI",
"cal.cap.title":"Kolu dimdik yukari getirip bas: sadece referansi tasir",
"cal.dir":" YON ", "cal.dir.title":"%1 ekseninin yonunu ters cevir",
"cal.kin":"KINEMATIK: ", "cal.par":"PARALEL", "cal.ser":"SERI",
"cal.kin.title":"On kol omuzla birlikte donuyorsa SERI, dunyaya gore acisini koruyorsa PARALEL",
"cal.gripOpen":"KISKAC TAM ACIK", "cal.gripClose":"KISKAC TAM KAPALI", "cal.reset":"SIFIRLA",
"cal.sum.gain":"kazanc", "cal.slider.aria":"%1 servo acisi", "cal.quit":"CIKIS", "cal.clear":"ORNEKLERI SIL", "cal.nolink":"baglanti yok",

"cmd.engage":"ETKINLESTIR",
"cmd.engage.title":"Servolari kayitli pozda, kanal kanal devreye alir",
"cmd.home.title":"Kisayol: H", "cmd.park.title":"Kisayol: P",
"cmd.save":"POZU KAYDET", "cmd.save.title":"Mevcut pozu kalici hafizaya yazar",
"cmd.cal":"KALIBRASYON", "cmd.calEnd":"KALIBRASYONU BITIR",
"cmd.cal.title":"Adim adim kalibrasyon akisini baslatir",
"cmd.release":"SERBEST BIRAK", "cmd.release.title":"Tork kesilir, kol dusebilir",
"cmd.speed":"HIZ", "cmd.speed.aria":"Hiz olcegi",
"keys":"<kbd>Space</kbd> acil stop &nbsp;&middot;&nbsp; <kbd>H</kbd> home &nbsp;&middot;&nbsp; <kbd>P</kbd> park",
"foot.uptime":"calisma suresi", "foot.boot":"acilis sebebi",
"u.h":"s ", "u.m":"d ", "u.s":"sn",

"notice.known":"Servolar <b>serbest</b> (tork yok). Ekrandaki kol son kaydedilen durusu gosteriyor. Fiziksel kol elle oynatildiysa once ekrandaki kolu ona benzet, sonra <s>ETKINLESTIR</s>. Boylece devreye alirken hicbir sicrama olmaz.",
"notice.unknown":"<b>Kayitli poz yok.</b> Ekrandaki kolu fiziksel kolun su anki durusuna gore ayarla, sonra <s>ETKINLESTIR</s>. Aksi halde servolar devreye girerken ani hareket eder.",
"notice.estop":"<b>Acil stop aktif.</b> Konum korunuyor, hareket komutlari yok sayiliyor. Devam etmek icin <s>DEVAM ET</s> dugmesine bas.",

"confirm.release":"Servolar tamamen serbest kalacak. Kol yercekimiyle dusebilir. Devam?",
"confirm.capture":"Gercek kol SU AN dimdik yukari mi duruyor?\n(ust kol + on kol + kiskac tek dusey cizgi)\n\nBu poz referans olarak kaydedilecek.",
"confirm.calreset":"Kalibrasyon fabrika ayarina donecek. Emin misin?",

"w.engaging.h":"ADIM 1 &mdash; DEVREYE ALINIYOR",
"w.engaging.p":"<p>Kanallar akim piki olmasin diye <b>tek tek</b> aciliyor, bu ~1 saniye surer. Bitince kendiliginden 2. adima gecilecek.</p>",
"w.engaging.btn":"DEVREYE ALINIYOR...", "w.engaging.stat":"kanallar aciliyor",
"w.estop.h":"ADIM 1 &mdash; ACIL STOP AKTIF",
"w.estop.p":"<p>Kol acil stopta; hareket komutlari yok sayiliyor. Kalibrasyona devam etmek icin once <b>DEVAM ET</b>.</p>",
"w.estop.stat":"acil stop",
"w.arm.h":"ADIM 1 &mdash; KOLU ETKINLESTIR",
"w.arm.p":"<p>Servolar su an <b>serbest</b> (tork yok). Torksuz bir servoda &laquo;servo acisi&raquo; diye bir gercek yoktur &mdash; kol nereye birakildiysa oradadir &mdash; bu yuzden ornek alinamaz.</p><p>Etkinlestirmeden once ekrandaki kolu fiziksel kolun durusuna yaklastir ki devreye alirken sicrama olmasin.</p>",
"w.arm.stat":"servolar serbest",
"w.p1.h":"ADIM 2 &mdash; 1. POZU ESLE",
"w.p1.p":"<p><b>A &middot;</b> Asagidaki <b>Gercek kolu oynat</b> kaydiricilariyla kolu rahat bir poza getir.</p><p><b>B &middot;</b> Yan gorunusteki <s>turuncu</s> kolu <b>surukleyerek</b> gercek kolun gozunle gordugun durusuna benzet. Turuncu kol servoyu oynatmaz; sadece &laquo;kol gercekte boyle duruyor&raquo; dedigin bilgidir.</p>",
"w.p1.btn":"1. NOKTAYI KAYDET", "w.p1.stat":"turuncu kolu gercege benzet",
"w.moving":"kol hareket ediyor, dursun",
"w.p2.h":"ADIM 3 &mdash; 2. POZU ESLE",
"w.p2.p":"<p><b>A &middot;</b> Simdi kaydiricilarla eksenleri <b>oynat</b>. 1. noktadan <b>farkli</b> bir poz gerekiyor &mdash; ayni pozdan ikinci bir ornek hicbir sey ogretmez. Eksen basina en az %1&deg; (rahat olsun diye 15&deg;+). Her kaydiricinin sagindaki sayac 1. noktadan bu yana ne kadar oynadigini gosterir.</p><p><b>B &middot;</b> Turuncu kolu yeni durusa gore <b>tekrar esle</b>, sonra kaydet.</p>",
"w.p2.btn":"2. NOKTAYI KAYDET",
"w.p2.same":"kol hala 1. noktadaki pozda &mdash; once oynat",
"w.p2.ok":"<b>%1/3</b> eksen yeterince oynadi",
"w.fit.h":"ADIM 4 &mdash; HESAPLA",
"w.fit.p":"<p>Iki ornek hazir. Hesaplayinca her eksenin servo&nbsp;&rarr;&nbsp;aci donusumu (yon ve oran dahil) cozulup kalici hafizaya yazilir.</p>",
"w.fit.btn":"HESAPLA & KAYDET", "w.fit.stat":"iki ornek hazir",
"w.res.none":"Hicbir eksen guncellenmedi &mdash; eksenler iki poz arasinda yeterince oynatilmamis. Tekrar dene.",
"w.res.saved":"Kaydedildi:", "w.res.skip":" &nbsp;&middot;&nbsp; atlandi (az oynadi): ",

"rst.POWERON":"GUC VERILDI", "rst.EXT":"HARICI RESET", "rst.SW":"YAZILIM RESET",
"rst.PANIC":"PANIC (yazilim hatasi)", "rst.INT_WDT":"WATCHDOG (int)",
"rst.TASK_WDT":"WATCHDOG (gorev)", "rst.WDT":"WATCHDOG",
"rst.BROWNOUT":"BROWNOUT (besleme cokmesi!)", "rst.DEEPSLEEP":"DERIN UYKU",
"rst.SDIO":"SDIO", "rst.UNKNOWN":"BILINMIYOR"
},
en:{
"lang.switch":"Turkceye gec",
"badge.rst.title":"Last boot reason / reset counter",
"badge.idle":"● IDLE", "badge.moving":"▶ MOVING", "badge.reset":"RESET x",
"link.connecting":"CONNECTING", "link.on":"LINKED", "link.off":"OFFLINE",
"theme.light":"&#9788; LIGHT", "theme.dark":"&#9789; DARK",
"theme.toLight":"Switch to light theme", "theme.toDark":"Switch to dark theme",
"estop":"E-STOP", "estop.title":"Shortcut: Space", "resume":"RESUME",
"state.0":"DISARMED", "state.1":"ENGAGING...", "state.2":"ACTIVE", "state.3":"E-STOP",

"side.title":"Arm Plane — Side View",
"side.hint":"Drag a joint or the tip",
"side.hint.cal":"Match the ORANGE arm to the real arm",
"legend.model":'<i class="sw-model"></i>model arm &mdash; the pose the firmware assumes',
"legend.match":'<i class="sw-match"></i>orange arm &mdash; the real pose you matched',
"legend.note":"Dragging moves the ORANGE arm. Use the <b>sliders</b> below to move the real arm.",
"tg.level":"GRIPPER LEVEL", "tg.elbow.up":"ELBOW: UP", "tg.elbow.down":"ELBOW: DOWN",
"tg.ghost":"TARGET GHOST", "rd.tip":"TIP:",
"top.title":"Base Rotation", "top.hint":"Turn",
"grip.title":"Gripper", "grip.aria":"Gripper servo angle", "grip.openSuffix":"% OPEN",
"joints.title":"Axes", "joints.target":"TARGET",
"joints.hint":"large value = live position &nbsp;|&nbsp; white line = target",

"jn.0":"Base", "jn.1":"Shoulder", "jn.2":"Elbow", "jn.3":"Wrist", "jn.4":"Gripper",
"ax.0":"SHOULDER", "ax.1":"ELBOW", "ax.2":"WRIST",

"cal.title":"Calibration", "cal.open":"OPEN PANEL", "cal.close":"CLOSE",
"cal.step1":"Engage the arm", "cal.step2":"Match pose 1",
"cal.step3":"Match pose 2", "cal.step4":"Solve",
"cal.move.head":"Move the real arm <i>&mdash; these sliders drive the servos directly, not the orange arm</i>",
"cal.adv":"Advanced",
"cal.cap":"QUICK: STRAIGHT UP",
"cal.cap.title":"Bring the arm straight up, then press: moves the reference only",
"cal.dir":" DIR ", "cal.dir.title":"Flip the direction of the %1 axis",
"cal.kin":"KINEMATICS: ", "cal.par":"PARALLEL", "cal.ser":"SERIAL",
"cal.kin.title":"SERIAL if the forearm turns with the shoulder, PARALLEL if it holds its angle to the world",
"cal.gripOpen":"GRIPPER FULLY OPEN", "cal.gripClose":"GRIPPER FULLY CLOSED", "cal.reset":"RESET",
"cal.sum.gain":"gain", "cal.slider.aria":"%1 servo angle", "cal.quit":"EXIT", "cal.clear":"CLEAR SAMPLES", "cal.nolink":"no link",

"cmd.engage":"ENGAGE",
"cmd.engage.title":"Engages the servos at the saved pose, one channel at a time",
"cmd.home.title":"Shortcut: H", "cmd.park.title":"Shortcut: P",
"cmd.save":"SAVE POSE", "cmd.save.title":"Writes the current pose to persistent memory",
"cmd.cal":"CALIBRATION", "cmd.calEnd":"END CALIBRATION",
"cmd.cal.title":"Starts the step-by-step calibration flow",
"cmd.release":"RELEASE", "cmd.release.title":"Torque is cut, the arm may drop",
"cmd.speed":"SPEED", "cmd.speed.aria":"Speed scale",
"keys":"<kbd>Space</kbd> e-stop &nbsp;&middot;&nbsp; <kbd>H</kbd> home &nbsp;&middot;&nbsp; <kbd>P</kbd> park",
"foot.uptime":"uptime", "foot.boot":"boot reason",
"u.h":"h ", "u.m":"m ", "u.s":"s",

"notice.known":"The servos are <b>free</b> (no torque). The on-screen arm shows the last saved pose. If the physical arm was moved by hand, match the on-screen arm to it first, then <s>ENGAGE</s>. That way nothing jumps when torque comes on.",
"notice.unknown":"<b>No saved pose.</b> Set the on-screen arm to the physical arm's current pose, then <s>ENGAGE</s>. Otherwise the servos will jerk as they engage.",
"notice.estop":"<b>Emergency stop active.</b> Position is held, motion commands are ignored. Press <s>RESUME</s> to continue.",

"confirm.release":"The servos will go completely free. The arm may drop under gravity. Continue?",
"confirm.capture":"Is the real arm standing straight up RIGHT NOW?\n(upper arm + forearm + gripper in one vertical line)\n\nThis pose will be stored as the reference.",
"confirm.calreset":"Calibration will return to factory defaults. Are you sure?",

"w.engaging.h":"STEP 1 &mdash; ENGAGING",
"w.engaging.p":"<p>Channels come up <b>one by one</b> so the supply never sees a current spike; this takes about a second. Step 2 opens on its own when it finishes.</p>",
"w.engaging.btn":"ENGAGING...", "w.engaging.stat":"channels coming up",
"w.estop.h":"STEP 1 &mdash; E-STOP ACTIVE",
"w.estop.p":"<p>The arm is in emergency stop; motion commands are ignored. Press <b>RESUME</b> first to carry on calibrating.</p>",
"w.estop.stat":"emergency stop",
"w.arm.h":"STEP 1 &mdash; ENGAGE THE ARM",
"w.arm.p":"<p>The servos are <b>free</b> right now (no torque). On a torque-free servo there is no such thing as a &laquo;servo angle&raquo; &mdash; the arm simply rests wherever it was left &mdash; so no sample can be taken.</p><p>Before engaging, bring the on-screen arm close to the pose of the physical arm so nothing jumps as torque comes on.</p>",
"w.arm.stat":"servos are free",
"w.p1.h":"STEP 2 &mdash; MATCH POSE 1",
"w.p1.p":"<p><b>A &middot;</b> Use the <b>Move the real arm</b> sliders below to bring the arm into a comfortable pose.</p><p><b>B &middot;</b> <b>Drag</b> the <s>orange</s> arm in the side view until it looks like the pose you see on the real arm. The orange arm does not drive the servos; it is only you saying &laquo;the arm really looks like this&raquo;.</p>",
"w.p1.btn":"SAVE POINT 1", "w.p1.stat":"match the orange arm to reality",
"w.moving":"arm is moving, let it settle",
"w.p2.h":"STEP 3 &mdash; MATCH POSE 2",
"w.p2.p":"<p><b>A &middot;</b> Now <b>move</b> the axes with the sliders. The pose must be <b>different</b> from point 1 &mdash; a second sample from the same pose teaches nothing. At least %1&deg; per axis (15&deg;+ is comfortable). The counter beside each slider shows how far that axis has moved since point 1.</p><p><b>B &middot;</b> <b>Match</b> the orange arm to the new pose again, then save.</p>",
"w.p2.btn":"SAVE POINT 2",
"w.p2.same":"arm is still in the point 1 pose &mdash; move it first",
"w.p2.ok":"<b>%1/3</b> axes moved enough",
"w.fit.h":"STEP 4 &mdash; SOLVE",
"w.fit.p":"<p>Both samples are ready. Solving works out the servo&nbsp;&rarr;&nbsp;angle mapping of every axis (direction and ratio included) and writes it to persistent memory.</p>",
"w.fit.btn":"SOLVE & SAVE", "w.fit.stat":"two samples ready",
"w.res.none":"No axis was updated &mdash; the axes did not move enough between the two poses. Try again.",
"w.res.saved":"Saved:", "w.res.skip":" &nbsp;&middot;&nbsp; skipped (moved too little): ",

"rst.POWERON":"POWER ON", "rst.EXT":"EXTERNAL RESET", "rst.SW":"SOFTWARE RESET",
"rst.PANIC":"PANIC (firmware crash)", "rst.INT_WDT":"WATCHDOG (int)",
"rst.TASK_WDT":"WATCHDOG (task)", "rst.WDT":"WATCHDOG",
"rst.BROWNOUT":"BROWNOUT (supply collapsed!)", "rst.DEEPSLEEP":"DEEP SLEEP",
"rst.SDIO":"SDIO", "rst.UNKNOWN":"UNKNOWN"
}};

const LANGKEY = "ap.lang";
let lang = (function(){
  let s = null; try{ s = localStorage.getItem(LANGKEY); }catch(e){}
  if(s==="tr" || s==="en") return s;
  return /^tr/i.test(navigator.language || "") ? "tr" : "en";
})();

/* Anahtar sozlukte yoksa TR'ye, o da yoksa anahtarin kendisine duser: yeni bir
   metin cevirisini beklerken bile arayuz bos kutu gostermez. */
function L(k,a){
  const d = STR[lang] || STR.en;
  let s = (k in d) ? d[k] : ((k in STR.tr) ? STR.tr[k] : k);
  if(a!==undefined) s = s.replace("%1", a);
  return s;
}
/* Eksen adlari: TR'de config.h'tan gelen isimler kullanilir (mekanigi yeniden
   adlandirdiysan arayuzde de oyle gorunur), EN'de sozluk. */
const jname = i => (lang==="tr" && CFG.j[i]) ? CFG.j[i].n : L("jn."+i);
const axisNames = () => [L("ax.0"), L("ax.1"), L("ax.2")];


/* ===================== durum ===================== */
let CFG = {                                   // ESP32 baglandiginda uzerine yazilir
  fw:"ArmPilot",
  geo:{baseR:62,baseH:105,l1:110,l2:95,l3:72},
  j:[ {n:"Taban",lo:0,hi:180},{n:"Omuz",lo:5,hi:175},{n:"Dirsek",lo:0,hi:180},
      {n:"Bilek",lo:0,hi:180},{n:"Kiskac",lo:0,hi:180} ]
};
/* Kalibrasyon: her eklem icin  eklem_acisi = wref + gain*(servo - sref)
   ESP32'den ayri bir paketle gelir ve NVS'te kalicidir.
   smp[] = kalibrasyon modunda hangi esleme ornekleri kayitli. */
let CAL = { sref:[53.4,156.4,101.5], wref:[90,0,0], gain:[1,1,-1],
            gr:[20,95], par:0, smp:[0,0],
            s0:[90,90,90], s1:[90,90,90], fit:-1 };
/* calFit'in bir ekseni cozebilmesi icin gereken en az servo hareketi.
   Firmware'deki CAL_MIN_SPAN_DEG ile ayni olmali. */
const CAL_MIN_SPAN = 8;
let cmd  = [90,90,90,90,60];      // kullanicinin komut ettigi acilar
let live = [90,90,90,90,60];      // robotun anlik acilari (telemetri)
let tgt  = [90,90,90,90,60];      // robotun hedef acilari (telemetri)
let armState = 0, moving = false, poseKnown = false;
let ws = null, linked = false, lastSendAt = 0, dirty = false, dragging = false;
let linkState = 0;                /* 0 baglaniyor | 1 bagli | 2 kopuk */
let autoLevel = true, elbowUp = true, showGhost = true;
/* Kalibrasyon modu: MQ, kullanicinin "gercek kol su an boyle duruyor" diye
   ekranda esledigi EKLEM acilari (servo degil).
   matchTouched: kullanici turuncu kolu elle duzeltti mi? Duzeltmediyse turuncu
   kol modeli TAKIP eder -> kaydiricilarla kolu oynatinca ekranda da oynar.
   Elle duzeltince donar, yoksa kullanicinin emegi her telemetri paketinde
   silinirdi. */
let calMode = false, MQ = [90,0,0], matchTouched = false;
let NET = {}, rstText = "?", lastUp = -1, resetCount = 0;

const STATE_CLS = ["warn","acc","ok","bad"];

/* ===================== kinematik =====================
   Iki katmanli:
     servo acisi  <-> EKLEM acisi q   (kalibrasyon, dogrusal)
     eklem acisi  <-> DUNYA acisi t   (kinematik zincir)

   q[0] = ust kolun dunya acisi (90 = dimdik yukari)
   q[1] = seri kinematikte dirsegin bir onceki uzva GORE acisi (0 = duz),
          paralelde dunyada 90'dan sapma
   q[2] = bilek icin ayni mantik                                             */
function jointAngles(s){
  return [ CAL.wref[0] + CAL.gain[0]*(s[1]-CAL.sref[0]),
           CAL.wref[1] + CAL.gain[1]*(s[2]-CAL.sref[1]),
           CAL.wref[2] + CAL.gain[2]*(s[3]-CAL.sref[2]) ];
}
function worldFromJoint(q){
  const t1 = q[0];
  const t2 = CAL.par ? 90 + q[1] : t1 + q[1];
  const t3 = CAL.par ? 90 + q[2] : t2 + q[2];
  return [t1*D2R, t2*D2R, t3*D2R];
}
const worldAngles = s => worldFromJoint(jointAngles(s));

/* dunya acisi -> eklem acisi. Sonuc referans degerin +-180 komsulugunda
   tutulur, boylece sarma (wrap) yuzunden eksen ters tarafa firlamaz. */
function jointFromWorld(k, tDeg, parentDeg){
  const raw = (k===0) ? tDeg : (CAL.par ? tDeg-90 : tDeg-parentDeg);
  return CAL.wref[k] + norm180(raw - CAL.wref[k]);
}
/* eklem acisi -> servo acisi (kalibrasyonun tersi) */
function servoFromJoint(k,q){
  return CAL.sref[k] + (q - CAL.wref[k])/(CAL.gain[k] || 1);
}

function fkFromWorld(t){
  const g = CFG.geo, [t1,t2,t3] = t;
  const sh = {x:0, y:g.baseH};
  const eb = {x:sh.x+g.l1*Math.cos(t1), y:sh.y+g.l1*Math.sin(t1)};
  const wr = {x:eb.x+g.l2*Math.cos(t2), y:eb.y+g.l2*Math.sin(t2)};
  const tp = {x:wr.x+g.l3*Math.cos(t3), y:wr.y+g.l3*Math.sin(t3)};
  return {sh,eb,wr,tp,t1,t2,t3};
}
const fk = s => fkFromWorld(worldAngles(s));
/* Kalibrasyon modundaki "esleme kolu": kalibrasyondan gecmez, dogrudan
   kullanicinin esledigi eklem acilarindan cizilir. */
const fkMatch = () => fkFromWorld(worldFromJoint(MQ));

/* dunya acilarindan servo acilarina */
function toServo(t1,t2,t3){
  const d1 = t1*R2D, d2 = t2*R2D, d3 = t3*R2D;
  return [ servoFromJoint(0, jointFromWorld(0,d1,0)),
           servoFromJoint(1, jointFromWorld(1,d2,d1)),
           servoFromJoint(2, jointFromWorld(2,d3,d2)) ];
}
/* 2 uzuvlu analitik ters kinematik: ucu (x,y)'ye goturmeye calisir */
function ik(tx,ty,gripWorld){
  const g = CFG.geo;
  const wx = tx - g.l3*Math.cos(gripWorld);
  const wy = ty - g.l3*Math.sin(gripWorld);
  const dx = wx, dy = wy - g.baseH;
  let d = Math.hypot(dx,dy);
  d = clamp(d, Math.abs(g.l1-g.l2)+1, g.l1+g.l2-1);
  const phi = Math.atan2(dy,dx);
  const sgn = elbowUp ? 1 : -1;
  const a = Math.acos(clamp((d*d+g.l1*g.l1-g.l2*g.l2)/(2*d*g.l1),-1,1));
  const b = Math.acos(clamp((g.l1*g.l1+g.l2*g.l2-d*d)/(2*g.l1*g.l2),-1,1));
  const t1 = phi + sgn*a;
  const t2 = t1 - sgn*(Math.PI-b);
  return toServo(t1,t2,gripWorld);
}
function setJoint(i,v){
  cmd[i] = clamp(v, CFG.j[i].lo, CFG.j[i].hi);
  dirty = true;
}

/* ===================== yan gorunus cizimi ===================== */
const svgSide = document.getElementById("side");
let W = {}, gWorld = null, P = {};

function buildSide(){
  const g = CFG.geo;
  const R = g.l1 + g.l2 + g.l3, pad = 16;
  const w = 2*(R + g.baseR) + 2*pad;
  const h = g.baseH + R + pad + 30;
  W = {w,h,cx:w/2, cy:h-26};
  svgSide.setAttribute("viewBox", `0 0 ${w} ${h}`);
  svgSide.innerHTML = "";

  /* arka plan izgarasi */
  const bg = el("g",{});
  for(let x=-Math.ceil((W.cx)/40)*40; x<=w-W.cx; x+=40)
    bg.appendChild(el("line",{class:"grid-l",x1:W.cx+x,y1:0,x2:W.cx+x,y2:h}));
  for(let y=W.cy; y>=0; y-=40) bg.appendChild(el("line",{class:"grid-l",x1:0,y1:y,x2:w,y2:y}));
  svgSide.appendChild(bg);

  gWorld = el("g",{transform:`translate(${W.cx},${W.cy})`});
  svgSide.appendChild(gWorld);

  /* calisma zarfi */
  gWorld.appendChild(el("circle",{class:"envelope",cx:0,cy:-g.baseH,r:R}));
  gWorld.appendChild(el("circle",{class:"envelope",cx:0,cy:-g.baseH,r:g.l1+g.l2}));

  /* zemin */
  gWorld.appendChild(el("line",{class:"ground",x1:-W.cx+10,y1:0,x2:W.cx-10,y2:0}));
  for(let x=-W.cx+16;x<W.cx-16;x+=18)
    gWorld.appendChild(el("line",{class:"hatch",x1:x,y1:0,x2:x-10,y2:11}));

  /* taban govdesi (CAD siluetine yakin: alt silindir + donen taret + omuz yuvasi) */
  const bR=g.baseR, h1=g.baseH*0.42, h2=g.baseH;
  gWorld.appendChild(el("rect",{class:"base-body",x:-bR,y:-h1,width:2*bR,height:h1,rx:8}));
  gWorld.appendChild(el("rect",{class:"base-body",x:-bR*1.04,y:-h1-7,width:2*bR*1.04,height:11,rx:4}));
  gWorld.appendChild(el("rect",{class:"base-top",x:-bR*0.66,y:-h2+4,width:2*bR*0.66,height:h2-h1-4,rx:7}));
  gWorld.appendChild(el("rect",{class:"base-top",x:-bR*0.4,y:-h2-9,width:2*bR*0.4,height:16,rx:5}));

  /* --- hedef izi (ghost) --- */
  P.gh = el("g",{class:"ghost"});
  P.gh1=el("line",{}); P.gh2=el("line",{}); P.gh3=el("line",{});
  P.ghd1=el("circle",{class:"ghost-d",r:5}); P.ghd2=el("circle",{class:"ghost-d",r:5});
  P.ghd3=el("circle",{class:"ghost-d",r:4});
  [P.gh1,P.gh2,P.gh3,P.ghd1,P.ghd2,P.ghd3].forEach(n=>P.gh.appendChild(n));
  gWorld.appendChild(P.gh);

  /* --- gercek kol --- */
  P.arm = el("g",{});
  P.l1o = el("line",{class:"link link-o","stroke-width":30});
  P.l1i = el("line",{class:"link link-i","stroke-width":9});
  P.l2o = el("line",{class:"link link-o","stroke-width":25});
  P.l2i = el("line",{class:"link link-i","stroke-width":7});
  P.l3o = el("line",{class:"link link-o","stroke-width":15});
  P.f1  = el("path",{class:"finger","stroke-width":6});
  P.f2  = el("path",{class:"finger","stroke-width":6});
  P.d1  = el("circle",{class:"disc",r:21}); P.d1c = el("circle",{class:"disc-c",r:4});
  P.d2  = el("circle",{class:"disc",r:18}); P.d2c = el("circle",{class:"disc-c",r:3.5});
  P.d3  = el("circle",{class:"disc",r:11}); P.d3c = el("circle",{class:"disc-c",r:2.5});
  [P.l1o,P.l1i,P.l2o,P.l2i,P.l3o,P.f1,P.f2,P.d1,P.d1c,P.d2,P.d2c,P.d3,P.d3c]
    .forEach(n=>P.arm.appendChild(n));
  gWorld.appendChild(P.arm);

  /* --- esleme kolu (yalnizca kalibrasyon modunda) --- */
  P.mt = el("g",{class:"match",style:"display:none"});
  P.m1=el("line",{"stroke-width":9}); P.m2=el("line",{"stroke-width":7});
  P.m3=el("line",{"stroke-width":5});
  P.md1=el("circle",{class:"match-d",r:7}); P.md2=el("circle",{class:"match-d",r:6});
  P.md3=el("circle",{class:"match-d",r:5});
  [P.m1,P.m2,P.m3,P.md1,P.md2,P.md3].forEach(n=>P.mt.appendChild(n));
  gWorld.appendChild(P.mt);

  /* --- tutamaclar --- */
  P.hEb  = el("circle",{class:"handle",r:15});
  P.hWr  = el("circle",{class:"handle",r:13});
  P.hGr  = el("circle",{class:"handle",r:10});
  P.hTip = el("circle",{class:"handle tip",r:17});
  [P.hEb,P.hWr,P.hGr,P.hTip].forEach(n=>gWorld.appendChild(n));

  bindDrag(P.hEb ,"sh");
  bindDrag(P.hWr ,"el");
  bindDrag(P.hGr ,"wr");
  bindDrag(P.hTip,"ik");
}

const wx = p=>p.x, wy = p=>-p.y;                       /* dunya -> svg */
function seg(n,a,b){ n.setAttribute("x1",wx(a));n.setAttribute("y1",wy(a));
                     n.setAttribute("x2",wx(b));n.setAttribute("y2",wy(b)); }
function at(n,p){ n.setAttribute("cx",wx(p));n.setAttribute("cy",wy(p)); }

function drawSide(){
  const a = fk(live), c = fk(cmd), g = CFG.geo;

  /* gercek kol */
  seg(P.l1o,a.sh,a.eb); seg(P.l1i,a.sh,a.eb);
  seg(P.l2o,a.eb,a.wr); seg(P.l2i,a.eb,a.wr);
  const palm = {x:a.wr.x+g.l3*0.42*Math.cos(a.t3), y:a.wr.y+g.l3*0.42*Math.sin(a.t3)};
  seg(P.l3o,a.wr,palm);

  /* kiskac parmaklari */
  const openT = clamp((CAL.gr[1]-live[4])/(CAL.gr[1]-CAL.gr[0]||1),0,1);
  const spread = (3 + 20*openT)*D2R, fl = g.l3*0.62;
  for(const [node,s] of [[P.f1,1],[P.f2,-1]]){
    const d = a.t3 + s*spread;
    const knu = {x:palm.x+fl*0.42*Math.cos(d), y:palm.y+fl*0.42*Math.sin(d)};
    const end = {x:knu.x+fl*0.66*Math.cos(a.t3), y:knu.y+fl*0.66*Math.sin(a.t3)};
    node.setAttribute("d",`M${wx(palm)},${wy(palm)} L${wx(knu)},${wy(knu)} L${wx(end)},${wy(end)}`);
  }
  at(P.d1,a.sh); at(P.d1c,a.sh); at(P.d2,a.eb); at(P.d2c,a.eb); at(P.d3,a.wr); at(P.d3c,a.wr);
  P.arm.setAttribute("class", (armState===0 ? "limp" : "") + (calMode ? " dim" : ""));

  /* hedef izi (kalibrasyon modunda gizli: ekran zaten kalabalik) */
  const gap = Math.abs(cmd[1]-live[1])+Math.abs(cmd[2]-live[2])+Math.abs(cmd[3]-live[3]);
  P.gh.style.display = (showGhost && !calMode && gap>0.8) ? "" : "none";
  seg(P.gh1,c.sh,c.eb); seg(P.gh2,c.eb,c.wr); seg(P.gh3,c.wr,c.tp);
  at(P.ghd1,c.eb); at(P.ghd2,c.wr); at(P.ghd3,c.tp);

  /* esleme kolu + tutamaclar.
     Normal modda tutamaclar komut pozunda, kalibrasyon modunda esleme
     kolunun uzerinde durur (surukledigimiz sey odur). */
  P.mt.style.display = calMode ? "" : "none";
  const h = calMode ? fkMatch() : c;
  if(calMode){
    seg(P.m1,h.sh,h.eb); seg(P.m2,h.eb,h.wr); seg(P.m3,h.wr,h.tp);
    at(P.md1,h.eb); at(P.md2,h.wr); at(P.md3,h.tp);
  }
  const palmC = {x:h.wr.x+g.l3*0.5*Math.cos(h.t3), y:h.wr.y+g.l3*0.5*Math.sin(h.t3)};
  at(P.hEb,h.eb); at(P.hWr,h.wr); at(P.hGr,palmC); at(P.hTip,h.tp);
  P.hTip.style.display = calMode ? "none" : "";   /* cal modunda IK anlamsiz */

  document.getElementById("rdX").textContent = Math.round(c.tp.x);
  document.getElementById("rdY").textContent = Math.round(c.tp.y);
}

/* ---- surukleme ---- */
function toWorld(ev){
  const pt = svgSide.createSVGPoint();
  pt.x = ev.clientX; pt.y = ev.clientY;
  const p = pt.matrixTransform(gWorld.getScreenCTM().inverse());
  return {x:p.x, y:-p.y};
}
let dragGripWorld = 0;   /* IK surukleme boyunca sabit tutulan kiskac yonu */
function bindDrag(node, kind){
  node.addEventListener("pointerdown", ev=>{
    ev.preventDefault(); node.setPointerCapture(ev.pointerId);
    node.classList.add("act"); dragging = true; node.dataset.on = "1";
    dragGripWorld = fk(cmd).t3;
  });
  node.addEventListener("pointermove", ev=>{
    if(node.dataset.on!=="1") return;
    ev.preventDefault(); onDrag(kind, toWorld(ev));
  });
  const end = ev=>{
    if(node.dataset.on!=="1") return;
    node.dataset.on = ""; node.classList.remove("act"); dragging = false;
    try{ node.releasePointerCapture(ev.pointerId); }catch(e){}
    pushNow();
  };
  node.addEventListener("pointerup", end);
  node.addEventListener("pointercancel", end);
}
/* Tek eklemi surukle: tutamacin yeni dunya acisini o eksenin servo acisina
   cevirir. Zincirin geri kalanina dokunulmaz -> eksenler bagimsiz oynar. */
function dragJoint(k, tRad, parentRad){
  setJoint(k+1, servoFromJoint(k, jointFromWorld(k, tRad*R2D, parentRad*R2D)));
}
function onDrag(kind, m){
  /* --- kalibrasyon modu: ekrandaki kolu gercek kola benzetiyoruz.
         Burada servo komutu DEGISMEZ, sadece "kol gercekte boyle duruyor"
         bilgisi (MQ) guncellenir. --- */
  if(calMode){
    const c = fkMatch();
    if(kind==="sh")      MQ[0] = norm180(Math.atan2(m.y-c.sh.y, m.x-c.sh.x)*R2D);
    else if(kind==="el") MQ[1] = norm180(Math.atan2(m.y-c.eb.y, m.x-c.eb.x)*R2D
                                         - (CAL.par ? 90 : c.t1*R2D));
    else if(kind==="wr") MQ[2] = norm180(Math.atan2(m.y-c.wr.y, m.x-c.wr.x)*R2D
                                         - (CAL.par ? 90 : c.t2*R2D));
    else return;                       /* cal modunda IK yok */
    matchTouched = true;               /* artik modeli takip etme, elde kal */
    redraw();
    return;
  }

  const c = fk(cmd);
  if(kind==="sh"){
    dragJoint(0, Math.atan2(m.y-c.sh.y, m.x-c.sh.x), 0);
  }else if(kind==="el"){
    dragJoint(1, Math.atan2(m.y-c.eb.y, m.x-c.eb.x), c.t1);
  }else if(kind==="wr"){
    dragJoint(2, Math.atan2(m.y-c.wr.y, m.x-c.wr.x), c.t2);
  }else{
    const gw = autoLevel ? 0 : dragGripWorld;
    const s = ik(m.x, m.y, gw);
    setJoint(1,s[0]); setJoint(2,s[1]); setJoint(3,s[2]);
  }
  drawSide(); drawTop(); drawJoints(); push();
}

/* ===================== ust gorunus ===================== */
const svgTop = document.getElementById("top");
let T = {};
function buildTop(){
  svgTop.innerHTML = "";
  const cx=130, cy=132, R=104;
  T = {cx,cy,R};
  svgTop.appendChild(el("path",{class:"fan",d:`M${cx-R},${cy} A${R},${R} 0 0 1 ${cx+R},${cy} Z`}));
  svgTop.appendChild(el("path",{class:"dial",d:`M${cx-R},${cy} A${R},${R} 0 0 1 ${cx+R},${cy} Z`,fill:"none"}));
  for(let a=0;a<=180;a+=10){
    const maj = a%45===0, r0 = R-(maj?15:8);
    const t = (a-90)*D2R, dx=Math.sin(t), dy=-Math.cos(t);
    svgTop.appendChild(el("line",{class:maj?"tick maj":"tick",
      x1:cx+dx*r0,y1:cy+dy*r0,x2:cx+dx*R,y2:cy+dy*R}));
    if(maj) svgTop.appendChild(Object.assign(
      el("text",{class:"lbl","text-anchor":"middle",x:cx+dx*(R-27),y:cy+dy*(R-27)+4}),
      {textContent:a}));
  }
  T.beamG = el("line",{class:"beam-g"});
  T.beam  = el("line",{class:"beam"});
  T.tip   = el("circle",{class:"handle tip",r:11});
  T.hub   = el("circle",{class:"disc",cx:cx,cy:cy,r:17});
  T.hubc  = el("circle",{class:"disc-c",cx:cx,cy:cy,r:4});
  T.val   = el("text",{class:"lbl-a","text-anchor":"middle",x:cx,y:cy+26});
  [T.beamG,T.beam,T.hub,T.hubc,T.tip,T.val].forEach(n=>svgTop.appendChild(n));

}
/* Dinleyiciler svgTop uzerinde durur; buildTop tekrar cagrilinca cogalmasin
   diye ayri ve yalnizca bir kez baglanir. */
function bindTop(){
  const grab = ev=>{
    const r = svgTop.getBoundingClientRect();
    const px = (ev.clientX-r.left)*(260/r.width) - T.cx;
    const py = (ev.clientY-r.top)*(175/r.height) - T.cy;
    setJoint(0, 90 + Math.atan2(px,-py)*R2D);
    drawTop(); drawJoints(); push();
  };
  svgTop.addEventListener("pointerdown", ev=>{
    ev.preventDefault(); svgTop.setPointerCapture(ev.pointerId);
    dragging = true; svgTop.dataset.on="1"; grab(ev);
  });
  svgTop.addEventListener("pointermove", ev=>{ if(svgTop.dataset.on==="1"){ev.preventDefault();grab(ev);} });
  const end = ev=>{ if(svgTop.dataset.on!=="1")return; svgTop.dataset.on=""; dragging=false;
    try{svgTop.releasePointerCapture(ev.pointerId);}catch(e){} pushNow(); };
  svgTop.addEventListener("pointerup", end);
  svgTop.addEventListener("pointercancel", end);
}
function drawTop(){
  const {cx,cy,R} = T;
  const rr = clamp(Math.abs(fk(cmd).tp.x)/(CFG.geo.l1+CFG.geo.l2+CFG.geo.l3), .42, 1)*(R-14);
  const put=(node,ang,len)=>{ const t=(ang-90)*D2R;
    node.setAttribute("x1",cx);node.setAttribute("y1",cy);
    node.setAttribute("x2",cx+Math.sin(t)*len);node.setAttribute("y2",cy-Math.cos(t)*len); };
  const tl = (ang,len)=>{const t=(ang-90)*D2R;return[cx+Math.sin(t)*len, cy-Math.cos(t)*len];};
  put(T.beamG, live[0], rr);
  put(T.beam,  cmd[0],  rr);
  const [tx,ty] = tl(cmd[0], rr);
  T.tip.setAttribute("cx",tx); T.tip.setAttribute("cy",ty);
  T.val.textContent = cmd[0].toFixed(0)+"°";
}

/* ===================== kiskac ===================== */
const svgGrip = document.getElementById("grip");
let G = {};
function buildGrip(){
  svgGrip.innerHTML = "";
  G.f1 = el("path",{class:"finger","stroke-width":11});
  G.f2 = el("path",{class:"finger","stroke-width":11});
  G.body = el("rect",{class:"base-top",x:118,y:4,width:24,height:24,rx:5});
  [G.body,G.f1,G.f2].forEach(n=>svgGrip.appendChild(n));
  G.txt = el("text",{class:"lbl-a","text-anchor":"middle",x:130,y:114});
  svgGrip.appendChild(G.txt);
}
function drawGrip(){
  const openT = clamp((CAL.gr[1]-live[4])/(CAL.gr[1]-CAL.gr[0]||1),0,1);
  const half = 8 + 34*openT;
  G.f1.setAttribute("d",`M130,28 L${130-half},46 L${130-half},88`);
  G.f2.setAttribute("d",`M130,28 L${130+half},46 L${130+half},88`);
  G.txt.textContent = (openT*100).toFixed(0)+L("grip.openSuffix");
  document.getElementById("gripHint").textContent = cmd[4].toFixed(0)+"°";
  const gr = document.getElementById("gripRange");
  if(document.activeElement!==gr) gr.value = Math.round(cmd[4]);
}

/* ===================== eksen tablosu ===================== */
function buildJoints(){
  const box = document.getElementById("joints");
  box.innerHTML = "";
  CFG.j.forEach((j,i)=>{
    const c = document.createElement("div"); c.className="jcard";
    c.innerHTML =
      `<div class="jn">${jname(i)}</div>
       <div class="jv"><span id="a${i}">0</span><i>&deg;</i></div>
       <div class="jbar"><div class="fillc" id="f${i}"></div><div class="ghostm" id="m${i}"></div></div>
       <div class="jt">${L("joints.target")} <b id="t${i}">0</b>&deg; &nbsp; ${j.lo}&ndash;${j.hi}</div>`;
    box.appendChild(c);
  });
}
function drawJoints(){
  CFG.j.forEach((j,i)=>{
    const sp = j.hi-j.lo || 1;
    document.getElementById("f"+i).style.width = (clamp((live[i]-j.lo)/sp,0,1)*100)+"%";
    document.getElementById("m"+i).style.left  = "calc("+(clamp((cmd[i]-j.lo)/sp,0,1)*100)+"% - 1px)";
    document.getElementById("a"+i).textContent = live[i].toFixed(1);
    document.getElementById("t"+i).textContent = cmd[i].toFixed(0);
  });
}

/* ===================== kalibrasyon paneli ===================== */
function buildCalPanel(){
  const box = document.getElementById("calSliders");
  box.innerHTML = "";
  CFG.j.forEach((j,i)=>{
    const r = document.createElement("div"); r.className = "calrow";
    r.innerHTML = `<span>${jname(i)}</span>
      <input type="range" id="cr${i}" min="${j.lo}" max="${j.hi}" step="0.5"
             aria-label="${L("cal.slider.aria", jname(i))}">
      <b id="cv${i}">0</b>
      <b class="delta" id="cd${i}"></b>`;
    box.appendChild(r);
  });
  CFG.j.forEach((j,i)=>{
    const s = document.getElementById("cr"+i);
    s.addEventListener("input",e=>{
      setJoint(i, +e.target.value);
      /* Kol oynadi: onceki esleme gecersiz, turuncu kol yeniden takibe gecsin. */
      matchTouched = false;
      redraw(); push();
    });
    s.addEventListener("change", pushNow);
  });
}

/* Akisin hangi adiminda oldugumuz TEK bir yerden turetilir; hem serit, hem
   talimat, hem de birincil dugme bunu okur -> tutarsizlik olamaz.
     0 kol hazir degil | 1 ilk ornek | 2 ikinci ornek | 3 hesapla           */
function calStepNow(){
  if(armState!==2) return 0;
  if(!CAL.smp[0])  return 1;
  if(!CAL.smp[1])  return 2;
  return 3;
}
/* 1. ornekten bu yana her eksenin ne kadar oynadigi */
const calSpans = ()=>[0,1,2].map(i=>Math.abs(live[i+1]-CAL.s0[i]));

/* Son hesabin sonucu: hangi eksen guncellendi, hangisi atlandi */
function calResultHtml(){
  const m = CAL.fit;
  if(m===undefined || m<0) return "";
  if(m===0) return '<div class="res bad">'+L("w.res.none")+'</div>';
  const AX = axisNames();
  const done = AX.filter((x,i)=>m&(1<<i)), skip = AX.filter((x,i)=>!(m&(1<<i)));
  return '<div class="res ok">'+L("w.res.saved")+' <b>'+done.join(", ")+'</b>'+
         (skip.length ? L("w.res.skip")+skip.join(", ") : '')+'</div>';
}

/* Aktif adimin talimati + birincil dugmenin etiketi/durumu.
   Her an ekranda TEK bir "simdi sunu yap" olsun diye tek dugmeye indirgendi. */
function calGuideFor(st){
  if(st===0){
    if(armState===1) return {h:L("w.engaging.h"), p:L("w.engaging.p"),
      btn:L("w.engaging.btn"), dis:true, stat:L("w.engaging.stat")};
    if(armState===3) return {h:L("w.estop.h"), p:L("w.estop.p"),
      btn:L("resume"), dis:false, stat:L("w.estop.stat")};
    return {h:L("w.arm.h"), p:L("w.arm.p"),
      btn:L("cmd.engage"), dis:false, stat:L("w.arm.stat")};
  }
  /* Kol hareket ederken ornek almak yanlistir: kullanicinin gozuyle esledigi
     durus ile o an okunan servo acisi birbirini tutmaz. */
  if(st===1) return {h:L("w.p1.h"), p:L("w.p1.p"),
    btn:L("w.p1.btn"), dis:moving,
    stat: moving ? L("w.moving") : L("w.p1.stat")};

  if(st===2){
    const sp = calSpans(), ok = sp.filter(v=>v>=CAL_MIN_SPAN).length;
    return {h:L("w.p2.h"), p:L("w.p2.p", CAL_MIN_SPAN),
      btn:L("w.p2.btn"),
      dis: moving || ok===0,
      stat: moving ? L("w.moving")
          : ok===0 ? L("w.p2.same")
                   : L("w.p2.ok", ok)};
  }
  return {h:L("w.fit.h"), p:L("w.fit.p"),
    btn:L("w.fit.btn"), dis:false, stat:L("w.fit.stat")};
}

/* innerHTML yazmak tarayiciya dugumleri yeniden ayristirtir. Telemetri 20 Hz
   aktigi icin ayni metni her pakette yeniden yazmak hem bosuna is olur hem de
   kullanicinin panelde sectigi metni surekli bozar. Son yazilan degeri
   hatirlayip yalnizca gercekten degistiginde dokunuyoruz. */
const htmlWas = {};
function setHtml(id, html){
  if(htmlWas[id] === html) return;
  htmlWas[id] = html;
  document.getElementById(id).innerHTML = html;
}

/* Panel katlanmis olsa bile baslikta duran ozet: kalibrasyonun ozeti her zaman
   goz onunde kalsin diye drawCal'dan ayri tutuluyor. */
function drawCalSummary(){
  document.getElementById("calSum").textContent =
    `sref ${CAL.sref.map(v=>v.toFixed(0)).join("/")}  ·  ${L("cal.sum.gain")} ${CAL.gain.map(v=>v.toFixed(2)).join("/")}  ·  ${L(CAL.par?"cal.par":"cal.ser")}`;
}

function drawCal(){
  CFG.j.forEach((j,i)=>{
    const s = document.getElementById("cr"+i);
    if(!s) return;
    if(document.activeElement!==s) s.value = cmd[i];
    document.getElementById("cv"+i).textContent = cmd[i].toFixed(0);
  });
  const mb = document.getElementById("calKin");
  mb.textContent = L("cal.kin") + L(CAL.par ? "cal.par" : "cal.ser");
  mb.classList.toggle("on", !!CAL.par);
  const AX = axisNames();
  for(let i=0;i<3;i++){
    const b = document.getElementById("calD"+i);
    b.textContent = AX[i] + L("cal.dir") + (CAL.gain[i]>0 ? "+" : "−");
    b.title = L("cal.dir.title", AX[i]);
    b.classList.toggle("on", CAL.gain[i]<0);
  }
  /* --- sihirbaz --- */
  const st = calStepNow(), g = calGuideFor(st);

  /* Kaydiricilarin yanindaki hareket sayaci: yalnizca 2. ornegi beklerken
     anlamli. Kullanici "yeterince oynattim mi" sorusunu tam orada gorur. */
  for(let i=1;i<=3;i++){
    const d = document.getElementById("cd"+i);
    if(!d) continue;
    if(st===2){
      const v = Math.abs(live[i]-CAL.s0[i-1]);
      d.textContent = v.toFixed(0)+"°" + (v>=CAL_MIN_SPAN ? " ✓" : "");
      d.className = "delta" + (v>=CAL_MIN_SPAN ? " ok" : "");
    }else{
      d.textContent = "";
      d.className = "delta";
    }
  }

  for(let i=0;i<4;i++){
    const e = document.getElementById("cs"+i);
    e.classList.toggle("act", i===st);
    e.classList.toggle("done", i<st);
  }
  setHtml("calGuide", "<h3>"+g.h+"</h3>"+g.p);
  setHtml("calRes", calResultHtml());

  const pb = document.getElementById("calPrimary");
  pb.textContent = g.btn;
  pb.disabled = g.dis || !linked;
  setHtml("calStat", linked ? g.stat : L("cal.nolink"));
  document.getElementById("calClr").disabled = !(CAL.smp[0] || CAL.smp[1]);
}
function sendK(a){ if(linked) ws.send("k "+a); }
/* Esleme kolunu modelin su anki duruşundan baslat: kullanici sifirdan
   kurmak yerine kucuk duzeltmelerle gercege yaklastirir. */
function seedMatch(){ MQ = jointAngles(live).map(v=>norm180(v)); }

/* ===================== iletisim ===================== */
function push(){
  const now = performance.now();
  if(now-lastSendAt < 40) return;
  pushNow();
}
function pushNow(){
  if(!linked || !dirty) return;
  lastSendAt = performance.now(); dirty = false;
  ws.send("a "+cmd.map(v=>v.toFixed(1)).join(" "));
}
function sendCmd(c){ if(linked) ws.send("c "+c); }

function connect(){
  ws = new WebSocket(`ws://${location.hostname}/ws`);
  ws.onopen = ()=>{ linked=true; linkState=1; setBadge("bLink",L("link.on"),"ok"); };
  ws.onclose = ()=>{ linked=false; linkState=2; setBadge("bLink",L("link.off"),"bad"); setTimeout(connect,1500); };
  ws.onerror = ()=>{ try{ws.close();}catch(e){} };
  ws.onmessage = ev=>{
    let m; try{ m = JSON.parse(ev.data); }catch(e){ return; }
    if(m.t==="hello"){
      CFG = Object.assign(CFG,m.cfg);
      NET = m.cfg; rstText = m.cfg.rst || "?";
      document.getElementById("fwv").textContent = "v"+m.cfg.fw;
      cmd = m.cur.slice(); live = m.cur.slice(); tgt = m.cur.slice();
      buildSide(); buildTop(); buildGrip(); buildJoints(); buildCalPanel();
      redraw(); updateChrome();
    }else if(m.t==="cal"){
      const wasPar = CAL.par;
      CAL = {sref:m.sref, wref:m.wref, gain:m.gain, gr:m.gr, par:m.par,
             smp:m.smp||[0,0], s0:m.s0||[90,90,90], s1:m.s1||[90,90,90],
             fit:(m.fit===undefined?-1:m.fit)};
      /* Kalibrasyon degisti: esleme kolu eski modele gore duruyordu, yeni
         modelin duruşuna cek. Kinematik modu degistiyse zaten anlamini yitirir. */
      if(calMode && wasPar!==m.par) seedMatch();
      redraw();
    }else if(m.t==="s"){
      live = m.c; tgt = m.g; moving = !!m.m;
      const st = m.s, pk = !!m.k;
      if(st!==armState || pk!==poseKnown){ armState=st; poseKnown=pk; updateChrome(); }
      /* kullanici surukleme yapmiyorsa robot hedefini yansit (HOME/PARK vs.) */
      if(!dragging && performance.now()-lastSendAt > 600 && !dirty) cmd = tgt.slice();
      /* Turuncu kol, kullanici ona dokunmadigi surece gercek kolu takip eder;
         boylece kaydiriciyla kolu oynatinca ekranda da oynadigi gorulur. */
      if(calMode && !matchTouched && !dragging) seedMatch();
      document.getElementById("spdV").textContent = m.v.toFixed(2)+"x";
      const sp = document.getElementById("spd");
      if(document.activeElement!==sp) sp.value = Math.round(m.v*100);
      setBadge("bMove", moving?L("badge.moving"):L("badge.idle"), moving?"acc":"");
      trackUptime(m.up);
      redraw();
    }
  };
}
function setBadge(id,txt,cls){
  const b = document.getElementById(id);
  b.textContent = txt; b.className = "badge"+(cls?" "+cls:"");
}
function redraw(){
  if(!gWorld) return;
  drawSide(); drawTop(); drawGrip(); drawJoints();
  drawCalSummary();
  /* Panel de eylem cubugu da kapaliysa kalibrasyon arayuzunun geri kalaninin
     tek pikseli bile gorunmuyor demektir; 20 Hz telemetride gorunmeyen
     dugumleri guncellemeyelim. Panel acilirken setCalPanel/setCalMode zaten
     drawCal'i cagirip her seyi tazeler. */
  if(document.getElementById("cr0") && (calMode || calPanelOpen())) drawCal();
}

/* ESP32 resetlenirse calisma suresi geri sarar; servolar da serbest kalir.
   "Durduk yere etkinsiz oldu" vakalarini burada yakaliyoruz. */
function trackUptime(up){
  if(up===undefined) return;
  if(lastUp>=0 && up < lastUp){
    resetCount++;
    console.warn("ESP32 reset edildi (#"+resetCount+")");
  }
  lastUp = up;
  const h = Math.floor(up/3600), mn = Math.floor(up/60)%60, sc = up%60;
  const t = (h?h+L("u.h"):"") + mn + L("u.m") + String(sc).padStart(2,"0") + L("u.s");
  setBadge("bRst",
    resetCount ? `⟳ ${L("badge.reset")}${resetCount}` : `⟳ ${rstLabel()}`,
    resetCount ? "bad" : (/BROWNOUT|PANIC|WDT/.test(rstText) ? "warn" : ""));
  document.getElementById("foot").textContent =
    `${NET.host||"armpilot.local"}  |  ${NET.ip||"-"}  |  ${L("foot.uptime")} ${t}  |  ${L("foot.boot")}: ${rstLabel()}`;
}

function updateChrome(){
  const si = STATE_CLS[armState]!==undefined ? armState : 0;
  setBadge("bState", L("state."+si), STATE_CLS[si]);
  const disarmed = armState===0, estop = armState===3;
  document.getElementById("btnEngage").disabled  = !disarmed;
  document.getElementById("btnRelease").disabled = disarmed;
  document.getElementById("btnHome").disabled = armState!==2;
  document.getElementById("btnPark").disabled = armState!==2;
  document.getElementById("btnEstop").textContent = L(estop ? "resume" : "estop");

  const n = document.getElementById("notice"), nt = document.getElementById("noticeTxt");
  if(disarmed){
    n.classList.add("show");
    nt.innerHTML = L(poseKnown ? "notice.known" : "notice.unknown");
  }else if(estop){
    n.classList.add("show");
    nt.innerHTML = L("notice.estop");
  }else{ n.classList.remove("show"); }
}

/* ---- dili uygula ----
   Statik nitelikler tek gecişte taranir; metni calisma aninda degisen dugumler
   (rozetler, mod dugmeleri) relabel() icinde elden gecirilir. Ikisi de dil
   degisiminde ve acilista bir kez kosar. */
function applyLang(){
  document.documentElement.lang = lang;
  const set = (sel,fn)=>document.querySelectorAll(sel).forEach(fn);
  set("[data-i18n]",       e=>{ e.textContent = L(e.getAttribute("data-i18n")); });
  set("[data-i18n-html]",  e=>{ e.innerHTML   = L(e.getAttribute("data-i18n-html")); });
  set("[data-i18n-title]", e=>{ e.title       = L(e.getAttribute("data-i18n-title")); });
  set("[data-i18n-aria]",  e=>{ e.setAttribute("aria-label", L(e.getAttribute("data-i18n-aria"))); });
  const b = document.getElementById("btnLang");
  b.textContent = lang==="tr" ? "EN" : "TR";
  b.title = L("lang.switch");
  relabel();
}
/* Durumu olan etiketler: hangi dile gecersek gecelim dogru varyant yazilsin. */
function relabel(){
  setBadge("bLink", L(linkState===1 ? "link.on" : linkState===2 ? "link.off" : "link.connecting"),
           linkState===1 ? "ok" : linkState===2 ? "bad" : "");
  setBadge("bMove", moving ? L("badge.moving") : L("badge.idle"), moving ? "acc" : "");
  document.getElementById("tgElbow").textContent  = L(elbowUp ? "tg.elbow.up" : "tg.elbow.down");
  document.getElementById("btnCal").textContent   = L(calMode ? "cmd.calEnd" : "cmd.cal");
  document.getElementById("calOpen").textContent  = L(calPanelOpen() ? "cal.close" : "cal.open");
  document.getElementById("sideHint").textContent = L(calMode ? "side.hint.cal" : "side.hint");
  applyTheme(theme);
  updateChrome();
}
/* Firmware acilis sebebini dile bagli olmayan bir KOD olarak yollar
   (POWERON, BROWNOUT, ...); okunur metne burada cevrilir. Tanimadigimiz bir
   kod gelirse kodun kendisi gosterilir. */
function rstLabel(){
  const k = "rst."+rstText, v = L(k);
  return v===k ? rstText : v;
}

/* ===================== kontroller ===================== */
/* blur(): tiklanan dugme odakta kalirsa Space tusu hem dugmeyi hem de
   klavye kisayolunu (acil stop) tetikliyordu. */
const on = (id,fn)=>document.getElementById(id).addEventListener("click",ev=>{
  ev.currentTarget.blur(); fn(ev);
});
on("btnEngage" ,()=>sendCmd("engage"));
on("btnRelease",()=>{ if(confirm(L("confirm.release"))) sendCmd("release"); });
on("btnHome"   ,()=>sendCmd("home"));
on("btnPark"   ,()=>sendCmd("park"));
on("btnSave"   ,()=>sendCmd("save"));
on("btnEstop"  ,()=>sendCmd(armState===3 ? "resume" : "stop"));

/* ---- tema ----
   Secim tarayicida kalir (ESP32'nin NVS'ini mesgul etmeye deger bir ayar
   degil; ayrica her cihaz kendi ortam isigina gore secebilsin). Kullanici
   hic secim yapmadiysa isletim sisteminin tercihi izlenir. */
const THKEY = "ap.theme";
let theme = document.documentElement.getAttribute("data-theme") || "dark";
function applyTheme(t){
  theme = t;
  document.documentElement.setAttribute("data-theme",t);
  const b = document.getElementById("btnTheme");
  b.innerHTML = L(t==="light" ? "theme.dark" : "theme.light");
  b.title = L(t==="light" ? "theme.toDark" : "theme.toLight");
}
applyTheme(theme);
on("btnTheme",()=>{
  const t = theme==="light" ? "dark" : "light";
  try{ localStorage.setItem(THKEY,t); }catch(e){}
  applyTheme(t);
});
/* Kullanici elle secmediyse sistem gece moduna gecince arayuz de gecsin. */
matchMedia("(prefers-color-scheme:light)").addEventListener("change",e=>{
  let s=null; try{ s=localStorage.getItem(THKEY); }catch(err){}
  if(!s) applyTheme(e.matches ? "light" : "dark");
});

/* ---- dil ----
   Tema gibi tarayicida saklanir; her cihaz kendi dilini secebilir. Metni
   yeniden uretmek gereken her sey burada tazelenir: eksen kartlari ve
   kalibrasyon kaydiricilari isim tasidigi icin bastan kurulur, setHtml
   onbellegi de bosaltilir (yoksa ayni anahtar "degismedi" sanilirdi). */
on("btnLang",()=>{
  lang = (lang==="tr") ? "en" : "tr";
  try{ localStorage.setItem(LANGKEY,lang); }catch(e){}
  for(const k in htmlWas) delete htmlWas[k];
  applyLang();
  buildJoints(); buildCalPanel();
  if(lastUp>=0) trackUptime(lastUp);
  redraw();
});

on("tgLevel",e=>{ autoLevel=!autoLevel; e.target.classList.toggle("on",autoLevel); });
on("tgElbow",e=>{ elbowUp=!elbowUp; e.target.textContent=L(elbowUp?"tg.elbow.up":"tg.elbow.down");
                  e.target.classList.toggle("on",!elbowUp); });
on("tgGhost",e=>{ showGhost=!showGhost; e.target.classList.toggle("on",showGhost); redraw(); });
document.getElementById("tgGhost").classList.add("on");

document.getElementById("gripRange").addEventListener("input",e=>{
  setJoint(4, +e.target.value); drawGrip(); drawJoints(); push();
});
document.getElementById("gripRange").addEventListener("change",pushNow);

let spdAt = 0;
document.getElementById("spd").addEventListener("input",e=>{
  const v = (+e.target.value)/100;
  document.getElementById("spdV").textContent = v.toFixed(2)+"x";
  const now = performance.now();
  if(linked && now-spdAt > 90){ spdAt = now; ws.send("s "+v.toFixed(2)); }
});
document.getElementById("spd").addEventListener("change",e=>{
  if(linked) ws.send("s "+((+e.target.value)/100).toFixed(2));
});

/* Kalibrasyona uc giris noktasi var: komut cubugundaki KALIBRASYON dugmesi,
   panel basligindaki PANELI AC ve panelin icindeki KALIBRASYON MODU. Hepsi
   asagidaki iki fonksiyonu cagirir ki hangi yoldan girilirse girilsin
   arayuzun durumu tutarli kalsin. */
const calPanelOpen = ()=>document.getElementById("calBody").style.display !== "none";
function setCalPanel(open){
  document.getElementById("calBody").style.display = open ? "" : "none";
  const b = document.getElementById("calOpen");
  b.textContent = L(open ? "cal.close" : "cal.open");
  b.classList.toggle("on",open);
  if(open) drawCal();
}
function setCalMode(v){
  calMode = v;
  if(calMode){ matchTouched = false; seedMatch(); setCalPanel(true); }
  document.getElementById("sideLegend").hidden = !calMode;

  const cb = document.getElementById("btnCal");
  cb.classList.toggle("primary",calMode);
  cb.textContent = L(calMode ? "cmd.calEnd" : "cmd.cal");

  /* eylem cubugu ekranin altina sabitlenir: surukleme yukarida yapiliyor */
  document.getElementById("calActions").hidden = !calMode;
  document.body.classList.toggle("calon",calMode);
  document.getElementById("sideHint").textContent =
    L(calMode ? "side.hint.cal" : "side.hint");

  /* Talimat ve kaydiricilar burada; kullaniciyi adimin basina goturelim.
     Hareket hassasiyeti ayarli kullanicida ani kaydir. */
  if(calMode){
    const soft = !matchMedia("(prefers-reduced-motion: reduce)").matches;
    document.getElementById("calPanel")
            .scrollIntoView({behavior: soft?"smooth":"auto", block:"start"});
  }
  redraw();
}
on("calOpen",()=>setCalPanel(!calPanelOpen()));
on("btnCal" ,()=>setCalMode(!calMode));
on("calCap",()=>{ if(confirm(L("confirm.capture"))) sendK("c"); });
on("calD0",()=>sendK("d 0"));
on("calD1",()=>sendK("d 1"));
on("calD2",()=>sendK("d 2"));
on("calKin",()=>sendK("m "+(CAL.par?0:1)));
on("calGo",()=>sendK("g o"));
on("calGc",()=>sendK("g c"));
on("calRst",()=>{ if(confirm(L("confirm.calreset"))) sendK("r"); });

/* ---- kalibrasyon eylem cubugu ----
   Tek bir birincil dugme var; ne yapacagi bulundugumuz adima gore belirlenir.
   Boylece "sirasi gelmemis" bir dugmeye basip uyari yemek mumkun degil. */
on("calPrimary",()=>{
  switch(calStepNow()){
    case 0: sendCmd(armState===3 ? "resume" : "engage"); break;
    case 1: sendK("p 0 "+MQ.map(v=>v.toFixed(1)).join(" ")); break;
    case 2: sendK("p 1 "+MQ.map(v=>v.toFixed(1)).join(" ")); break;
    default: sendK("f");
  }
});
on("calClr" ,()=>sendK("x"));
on("calQuit",()=>setCalMode(false));

addEventListener("keydown",e=>{
  if(e.repeat) return;
  const t = (e.target && e.target.tagName) || "";
  if(t==="INPUT"||t==="BUTTON"||t==="TEXTAREA"||t==="SELECT") return; /* odakli kontrol */
  if(e.code==="Space"){ e.preventDefault(); sendCmd(armState===3?"resume":"stop"); }
  else if(e.key==="h"||e.key==="H") sendCmd("home");
  else if(e.key==="p"||e.key==="P") sendCmd("park");
});

buildSide(); buildTop(); bindTop(); buildGrip(); buildJoints(); buildCalPanel();
applyLang(); redraw(); updateChrome(); connect();
</script>
)PAGE";
