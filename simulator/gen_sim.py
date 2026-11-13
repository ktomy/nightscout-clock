#!/usr/bin/env python3
"""Generate the nightscout-clock face simulator as a single self-contained HTML file."""
import os
import json

HERE = os.path.dirname(os.path.abspath(__file__))
AW = json.load(open(os.path.join(HERE, 'fonts', 'font_awtrix.json')))
MU = json.load(open(os.path.join(HERE, 'fonts', 'font_mu.json')))

# ---- static data transcribed from firmware source ----
ARROWS = {
    'DOUBLE_UP':    [0b01010000,0b11111000,0b01010000,0b01010000,0b01010000],
    'SINGLE_UP':    [0b00100000,0b01110000,0b10101000,0b00100000,0b00100000],
    'FORTYFIVE_UP': [0b00111000,0b00011000,0b00101000,0b01000000,0b10000000],
    'FLAT':         [0b00100000,0b00010000,0b11111000,0b00010000,0b00100000],
    'FORTYFIVE_DOWN':[0b10000000,0b01000000,0b00101000,0b00011000,0b00111000],
    'SINGLE_DOWN':  [0b00100000,0b00100000,0b10101000,0b01110000,0b00100000],
    'DOUBLE_DOWN':  [0b01010000,0b01010000,0b01010000,0b11111000,0b01010000],
    'NONE':         [0,0,0,0,0],
    'OLD':          [0b10001000,0b01010000,0b00100000,0b01010000,0b10001000],
}
# Rainbow mood smiley frames, transcribed from BGDisplayFaceRainbowMood.cpp.
# Each expression swaps between its two frames; rows are 8-bit bitmaps.
MOOD = {
    'HAPPY_A': [0b00111100,0b01000010,0b10100101,0b10000001,0b10100101,0b10011001,0b01000010,0b00111100],
    'HAPPY_B': [0b00111100,0b01000010,0b10100101,0b10000001,0b10000001,0b10111101,0b01000010,0b00111100],
    'SHOCKED_A': [0b00111100,0b01000010,0b10100101,0b10100101,0b10000001,0b10011001,0b01011010,0b00111100],
    'SHOCKED_B': [0b00111100,0b01000010,0b10100101,0b10100101,0b10000001,0b10011001,0b01000010,0b00111100],
    'DIZZY_A': [0b00111100,0b01000010,0b10011001,0b10100101,0b10000001,0b01011010,0b01000010,0b00111100],
    'DIZZY_B': [0b00111100,0b01000010,0b10100101,0b10011001,0b10000001,0b01011010,0b01000010,0b00111100],
}
# Upstream unicorn sprite (12x8, palette indices), palettes in RGB565
UNICORN_SPRITE = [
    1,1,0,0,0,0,0,0,0,0,0,0,
    0,1,1,0,0,4,5,6,7,0,0,0,
    0,0,1,1,4,4,4,5,6,7,0,0,
    0,0,0,2,2,2,2,4,5,6,7,0,
    2,2,2,2,3,2,2,4,5,6,7,0,
    2,2,2,2,2,2,2,4,5,6,7,0,
    0,2,2,2,2,2,2,4,5,6,7,0,
    0,0,0,2,2,2,2,2,4,5,6,7,
]
PAL_N = [0xFE87,0xF79D,0x18C3,0x4D5F,0x5EAD,0xFCC7,0xFA78,0x9AFE]
JS_DATA = "const AW=%s;\nconst MU=%s;\nconst ARROWS=%s;\nconst MOOD=%s;\nconst USPRITE=%s;\nconst UPALN=%s;\n" % (
    json.dumps(AW, separators=(',',':')),
    json.dumps(MU, separators=(',',':')),
    json.dumps(ARROWS, separators=(',',':')),
    json.dumps(MOOD, separators=(',',':')),
    json.dumps(UNICORN_SPRITE, separators=(',',':')),
    json.dumps(PAL_N, separators=(',',':')),
)

JS_ENGINE = r"""
// ================= engine =================
const C={BLACK:0,BLUE:0x001F,GREEN:0x07E0,CYAN:0x07FF,GRAY:0xA514,RED:0xF800,MAGENTA:0xF81F,YELLOW:0xFFE0,WHITE:0xFFFF};
const S={value:142,trend:'FLAT',ageMin:3,noData:false,mmol:false,h12:false,stale:0xA514,battery:87};
const cssCache={};
function css(c){c|=0;let s=cssCache[c];if(s)return s;
  const r=(c>>11)&31,g=(c>>5)&63,b=c&31;
  s=`rgb(${(r*255/31)|0},${(g*255/63)|0},${(b*255/31)|0})`;cssCache[c]=s;return s;}
function rgb565(r,g,b){return (((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3))|0;}
function hsv(h){h&=255;const region=(h/43)|0,rem=(h-region*43)*6,q=255-rem,t=rem;
  switch(region){case 0:return rgb565(255,t,0);case 1:return rgb565(q,255,0);case 2:return rgb565(0,255,t);
  case 3:return rgb565(0,q,255);case 4:return rgb565(t,0,255);default:return rgb565(255,0,q);}}
function fade(c){let r=(c>>11)&31,g=(c>>5)&63,b=c&31;
  r=(r*4/5)|0;g=(g*4/5)|0;b=(b*4/5)|0;return (r<<11)|(g<<5)|b;}
function px(b,x,y,c){if(x>=0&&x<32&&y>=0&&y<8)b[y*32+x]=c|0;}
function bitmap(b,x,y,bytes,w,h,color){
  // Adafruit GFX drawBitmap semantics: one byte per row, MSB = leftmost pixel.
  for(let r=0;r<h;r++)for(let c=0;c<w;c++){
    if((bytes[r]>>(7-c))&1)px(b,x+c,y+r,color);}}
// ---- text: Adafruit GFX custom-font semantics, pixel exact ----
function charW(F,ch){const w=F.charMap[ch.charCodeAt(0)];return w===undefined?4:w;}
function textW(F,s){let w=0;for(const ch of s)w+=charW(F,ch);return w;}
function drawText(b,F,s,x,y,align,color){ // align: 0 LEFT, 1 RIGHT, 2 CENTER
  const w=textW(F,s);
  let cx=align===0?x:align===1?x-w:Math.trunc((32-w)/2);
  for(const ch of s){
    const gi=ch.charCodeAt(0)-F.first;
    if(gi>=0&&gi<F.glyphs.length){
      const g=F.glyphs[gi],off=g[0],gw=g[1],gh=g[2],adv=g[3],xo=g[4],yo=g[5];
      for(let r=0;r<gh;r++)for(let c=0;c<gw;c++){const i=r*gw+c;
        if((F.bitmaps[off+(i>>3)]>>(7-(i&7)))&1)px(b,cx+xo+c,y+yo+r,color);}
      cx+=adv;
    } else cx+=4;
  }
}
// ---- glucose model ----
function level(sgv){if(sgv<55)return 'UL';if(sgv<70)return 'WL';if(sgv<=180)return 'N';if(sgv<250)return 'WH';return 'UH';}
function levelColor(sgv){return bandColor(level(sgv));}
function bandColor(lvl){return lvl==='N'?C.GREEN:((lvl==='WL'||lvl==='WH')?C.YELLOW:((lvl==='UL'||lvl==='UH')?C.RED:C.GRAY));}
function dim80(c){return ((((c>>11)&31)*0.8|0)<<11)|((((c>>5)&63)*0.8|0)<<5)|(((c&31)*0.8|0));}
function warnQuarter(sgv,lvl){
  const low=lvl==='WL';
  const dist=low?70-sgv:sgv-180, width=low?15:70;
  return width>0?Math.min(3,Math.max(0,Math.trunc(4*dist/width))):1;
}
function motionColor(lvl,quarter,index,frame){
  const pos=index+frame, third=pos%3===0;
  let shown=lvl, light=false;
  if(lvl==='UL'||lvl==='UH'){if(third)return 0;}
  else if(lvl==='WL'||lvl==='WH'){
    const urg=lvl==='WL'?'UL':'UH';
    if(quarter<=1)light=third;
    else if(quarter===2)shown=third?urg:lvl;
    else shown=pos%2===0?urg:lvl;
  }
  const col=bandColor(shown);
  return light?dim80(col):col;
}
function printable(sgv){return S.mmol?(sgv/18).toFixed(1):String(sgv);}
let READINGS=[];
function buildReadings(){
  if(S.noData){READINGS=[];return;}
  const per5={DOUBLE_UP:12,SINGLE_UP:6,FORTYFIVE_UP:3,FLAT:0,FORTYFIVE_DOWN:-3,SINGLE_DOWN:-6,DOUBLE_DOWN:-12,NONE:0}[S.trend];
  const arr=[];let v=S.value;
  for(let i=0;i<36;i++){
    arr.unshift({sgv:Math.round(v),ageSec:S.ageMin*60+i*300,trend:i===0?S.trend:'FLAT'});
    v-=per5;v+=Math.sin(i*1.7)*1.5;v=Math.max(40,Math.min(400,v));
  }
  READINGS=arr;
}
function isOld(){const l=READINGS[READINGS.length-1];return l?l.ageSec>=20*60:false;}
function trendArrow(b,rd,x,y,old,cbr){
  if(old){bitmap(b,x,y,ARROWS.OLD,5,5,S.stale);return;}
  bitmap(b,x,y,ARROWS[rd.trend]||ARROWS.NONE,5,5,cbr?levelColor(rd.sgv):C.WHITE);
}
function c565(c){return [((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31];}
function blend(a,b2,amt){const A=c565(a),B=c565(b2),k=255-amt;
  return rgb565((A[0]*k+B[0]*amt)/255,(A[1]*k+B[1]*amt)/255,(A[2]*k+B[2]*amt)/255);}
function trendVLine(b,x,trend,old){
  if(old)trend='NONE';
  const P=(yy,c)=>px(b,x,yy,c);
  switch(trend){
    case 'DOUBLE_UP':P(0,C.RED);P(1,C.YELLOW);P(2,C.GREEN);P(3,C.WHITE);break;
    case 'DOUBLE_DOWN':P(4,C.WHITE);P(5,C.GREEN);P(6,C.YELLOW);P(7,C.RED);break;
    case 'SINGLE_UP':P(1,C.YELLOW);P(2,C.GREEN);P(3,C.WHITE);break;
    case 'SINGLE_DOWN':P(4,C.WHITE);P(5,C.GREEN);P(6,C.YELLOW);break;
    case 'FORTYFIVE_UP':P(2,C.GREEN);P(3,C.WHITE);break;
    case 'FORTYFIVE_DOWN':P(4,C.WHITE);P(5,C.GREEN);break;
    case 'FLAT':P(3,C.WHITE);P(4,C.WHITE);break;
  }
}
function timerBlocks(b,last,width,xPos,yPos){
  const MAX=5;let n=(last.ageSec/60)|0;if(n>MAX)n=MAX;if(n<=0)return;
  const bs=((width-4)/MAX)|0;if(bs<1)return;
  xPos+=Math.trunc((width-(bs*MAX+(MAX-1)))/2);
  let col=C.GREEN;
  if(last.ageSec>=60*20)col=S.stale;else if(last.ageSec>=(MAX+1)*60)col=C.YELLOW;
  for(let i=0;i<n;i++)for(let j=0;j<bs;j++)px(b,xPos+i*(bs+1)+j,yPos,col);
}
function graph(b,x0,len,forMin){
  const ps=(forMin*60)/len;
  for(let i=0;i<len;i++){
    let sum=0,n=0;
    for(const r of READINGS){if(r.ageSec>=i*ps&&r.ageSec<(i+1)*ps){sum+=r.sgv;n++;}}
    if(!n)continue;
    const avg=sum/n,l=level(avg);let yy,col;
    if(l==='UH'){yy=0;col=C.RED;}else if(l==='WH'){yy=1;col=C.YELLOW;}
    else if(l==='N'){yy=5-Math.trunc((avg-70)*4/(180-70));col=C.GREEN;}
    else if(l==='WL'){yy=6;col=C.YELLOW;}else{yy=7;col=C.RED;}
    px(b,x0+len-1-i,yy,col);
  }
}
function noDataText(){return S.mmol?"--.-":"---";}
"""

JS_FACES = r"""
// ================= faces =================
// ---- animation-face helpers (transcribed from BGDisplayFaceRaceCar/Dragon) ----
const CAR_ART=["....HH...","BBBBBBB..","BBBBBBBBB"];
const CAR_OFF=[0,4,2,5];
function raceCar(b,sgv,old,frame){
  const lvl=old?'N':level(sgv);
  const q=old?0:warnQuarter(sgv,lvl);
  const put=(x,y,c)=>{if(!old)px(b,x,y,c);else if((x+y)%2===0)px(b,x,y,S.stale);};
  for(let line=0;line<4;line++)for(let x=0;x<7;x++){
    if((x+frame+CAR_OFF[line])%7>=3)continue;
    put(x,2+line,motionColor(lvl,q,6-x+line,frame));
  }
  for(let x=0;x<16;x++)if((x+frame)%4<2)put(x,7,C.BLUE);
  for(let row=0;row<3;row++)for(let col=0;col<9;col++){
    const cell=CAR_ART[row][col];
    if(cell!=='.')put(7+col,2+row,cell==='H'?C.MAGENTA:C.WHITE);
  }
  const spin=frame%2===1;
  for(const w of [8,13]){put(w,5,spin?C.CYAN:C.BLUE);put(w+1,5,spin?C.BLUE:C.CYAN);}
}
const DRAGON_ART=[
".WW...H.........",
"WWWW.SSH......t.",
".WWWSSESS...22.t",
"..SSSSSST.22112.",
"S.SSSSS.00011122",
"SSDDSSST.11122.t",
".S.S.S.....22.t.",
"....S..........."];
const IN_RANGE=[0xF81F,0xE01F,0xB19F,0x781F,0x401F,0x1C9F];
function dragonColor(cell){return cell==='S'?0x023F:cell==='D'?0xA79F:cell==='W'?0x073F:C.WHITE;}
function dragon(b,last,old,frame){
  const sgv=last.sgv, lvl=level(sgv), q=warnQuarter(sgv,lvl);
  const inRange=lvl==='N';
  const roll=Math.floor(frame/2)%3, phase=(frame%12)*(2*Math.PI/12);
  for(let row=0;row<8;row++)for(let col=0;col<16;col++){
    const cell=DRAGON_ART[row][col];
    if(cell==='.'||cell==='t'||(cell>='0'&&cell<='2'))continue;
    if(!old)px(b,col,row,dragonColor(cell));
    else if((row+col)%2===0)px(b,col,row,cell==='E'?0x18C3:S.stale);
  }
  if(old)return;
  for(let row=0;row<8;row++)for(let col=0;col<16;col++){
    const cell=DRAGON_ART[row][col];
    if(cell!=='t'&&(cell<'0'||cell>'2'))continue;
    let layer=cell==='t'?3:(((cell.charCodeAt(0)-48-roll)%3)+3)%3;
    const flick=Math.sin(phase-col*0.9+row*1.3);
    if(flick>0.7)layer=Math.max(0,layer-1);
    else if(flick<-0.7)layer++;
    if(layer>3)continue;
    let dr=row;
    if(layer===3){const bob=Math.sin(phase-col*0.8-row);
      dr=Math.min(7,Math.max(0,row+(bob>0.5?-1:bob<-0.5?1:0)));}
    px(b,col,dr,inRange?IN_RANGE[layer]:motionColor(lvl,q,15-col,frame));
  }
}
const FACES=[
{name:"Simple",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  if(!last){drawText(b,AW,noDataText(),0,6,2,S.stale);return;}
  drawText(b,AW,printable(last.sgv),0,6,2,old?S.stale:levelColor(last.sgv));
  trendArrow(b,last,27,1,old);
  timerBlocks(b,last,32,0,7);
}},
{name:"Full graph",draw(b,t){graph(b,0,32,180);}},
{name:"Graph and BG",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  if(!last){drawText(b,AW,noDataText(),30,6,1,S.stale);return;}
  const p=printable(last.sgv),tw=textW(AW,p),gw=32-tw-2;
  graph(b,0,gw,gw*5);
  drawText(b,AW,p,30,6,1,old?S.stale:levelColor(last.sgv));
  trendVLine(b,31,last.trend,old);
  timerBlocks(b,last,tw+2,gw,7);
}},
{name:"Big text",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  const s=last?printable(last.sgv):noDataText();
  drawText(b,MU,s,0,7,0,!last||old?S.stale:levelColor(last.sgv));
  if(last)trendArrow(b,last,27,1,old);
}},
{name:"Value and diff",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  if(!last){drawText(b,AW,noDataText(),13,6,1,S.stale);return;}
  const x=(S.mmol&&last.sgv>=180)?14:13;
  drawText(b,AW,printable(last.sgv),x,6,1,old?S.stale:levelColor(last.sgv));
  trendArrow(b,last,13,1,old);
  let diff="?";
  if(READINGS.length>=2){const d=last.sgv-READINGS[READINGS.length-2].sgv;
    diff=Math.abs(d)>99?"?":((d>=0?"+":"")+printable(d));}
  drawText(b,AW,diff,33,6,1,old?S.stale:C.WHITE);
  timerBlocks(b,last,32,0,7);
}},
{name:"Clock and value",draw(b,t){
  const now=new Date();let h=now.getHours();const m=now.getMinutes();
  if(S.h12){const pm=h>=12;h=h%12===0?12:h%12;for(let i=0;i<16;i++)px(b,i,7,pm?C.BLUE:C.CYAN);}
  drawText(b,AW,String(h).padStart(2,'0'),0,6,0,C.WHITE);
  drawText(b,AW,String(m).padStart(2,'0'),9,6,0,C.WHITE);
  const last=READINGS[READINGS.length-1],old=isOld();
  if(last){drawText(b,AW,printable(last.sgv),31,6,1,old?S.stale:levelColor(last.sgv));
    if(S.h12)timerBlocks(b,last,15,18,7);else timerBlocks(b,last,32,0,7);
    trendVLine(b,31,last.trend,old);
  }else drawText(b,AW,noDataText(),33,6,1,S.stale);
}},
{name:"Unicorn",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  const l=last?level(last.sgv):'N';
  const pal=(!last||old)?[S.stale,S.stale,0x18C3,S.stale,S.stale,S.stale,S.stale,S.stale]
    :(l==='N'?UPALN:(l==='WL'||l==='WH')?[0xFE87,0xF79D,0x18C3,C.YELLOW,C.YELLOW,C.YELLOW,C.YELLOW,C.YELLOW]
    :[0xFE87,0xF79D,0x18C3,C.RED,C.RED,C.RED,C.RED,C.RED]);
  for(let yy=0;yy<8;yy++)for(let xx=0;xx<12;xx++){const idx=USPRITE[yy*12+xx];if(idx)px(b,xx,yy,pal[idx-1]);}
  drawText(b,AW,last?printable(last.sgv):noDataText(),31,6,1,!last||old?S.stale:levelColor(last.sgv));
  if(last)timerBlocks(b,last,16,16,7);
}},
{name:"Time only",draw(b,t){
  const now=new Date();let h=now.getHours();const m=now.getMinutes(),s=now.getSeconds();let txt;
  if(S.h12){const ap=h<12?"AM":"PM";h=h%12===0?12:h%12;txt=`${h}:${String(m).padStart(2,'0')} ${ap}`;}
  else txt=`${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(s).padStart(2,'0')}`;
  drawText(b,AW,txt,0,6,2,C.WHITE);
}},
{name:"Simple (dark)",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  if(!last){drawText(b,AW,noDataText(),0,6,2,S.stale);return;}
  const lvl=level(last.sgv), urg=lvl==='UL'||lvl==='UH';
  drawText(b,AW,printable(last.sgv),0,6,2,old?S.stale:(urg?bandColor(lvl):C.WHITE));
  trendArrow(b,last,27,1,old,true);
}},
{name:"Race car",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  if(!last){drawText(b,AW,noDataText(),0,6,2,S.stale);return;}
  raceCar(b,last.sgv,old,old?0:Math.floor(t/110));
  drawText(b,AW,printable(last.sgv),31,6,1,old?S.stale:levelColor(last.sgv));
  timerBlocks(b,last,16,16,7);
}},
{name:"Dragon",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  if(!last){drawText(b,AW,noDataText(),0,6,2,S.stale);return;}
  dragon(b,last,old,old?0:Math.floor(t/110));
  drawText(b,AW,printable(last.sgv),31,6,1,old?S.stale:levelColor(last.sgv));
  timerBlocks(b,last,16,16,7);
}},
{name:"Big text (dark)",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  const s=last?printable(last.sgv):noDataText();
  const lvl=last?level(last.sgv):'N', urg=lvl==='UL'||lvl==='UH';
  drawText(b,MU,s,0,7,0,!last||old?S.stale:(urg?bandColor(lvl):C.WHITE));
  if(last)trendArrow(b,last,27,1,old,true);
}},
{name:"Rainbow clock",draw(b,t){
  const now=new Date();let txt;
  if(S.h12){let h=now.getHours();const ap=h<12?"AM":"PM";h=h%12===0?12:h%12;
    txt=`${h}:${String(now.getMinutes()).padStart(2,'0')} ${ap}`;}
  else txt=`${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}:${String(now.getSeconds()).padStart(2,'0')}`;
  const hueOff=Math.trunc(t/20)%256, len=txt.length;
  let x=Math.trunc((32-textW(AW,txt))/2);
  for(let i=0;i<len;i++){
    const ch=txt[i], hue=(Math.trunc(i*255/Math.max(len,1))+hueOff)&255;
    drawText(b,AW,ch,x,6,0,hsv(hue));
    x+=textW(AW,ch);
  }
}},
{name:"Rainbow big text",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  if(!last){drawText(b,MU,noDataText(),0,7,0,S.stale);return;}
  if(old){
    drawText(b,MU,printable(last.sgv),0,7,0,S.stale);
    trendArrow(b,last,27,1,true);
    return;
  }
  const s=printable(last.sgv), hueOff=Math.trunc(t/20)%256, len=s.length;
  let x=0;
  for(let i=0;i<len;i++){
    const ch=s[i], hue=(Math.trunc(i*255/Math.max(len,1))+hueOff)&255;
    drawText(b,MU,ch,x,7,0,hsv(hue));
    x+=textW(MU,ch);
  }
  trendArrow(b,last,27,1,false,true);
}},
{name:"Rainbow mood",draw(b,t){
  const last=READINGS[READINGS.length-1],old=isOld();
  if(!last){drawText(b,AW,noDataText(),0,6,2,S.stale);return;}
  const lvl=level(last.sgv);
  const falling=last.trend==='DOUBLE_DOWN'||last.trend==='SINGLE_DOWN';
  const first=Math.floor(t/300)%2===0;
  let bmp=first?MOOD.HAPPY_A:MOOD.HAPPY_B;
  if(lvl==='UL'||lvl==='WL'||falling)bmp=first?MOOD.SHOCKED_A:MOOD.SHOCKED_B;
  if(lvl==='UH'||lvl==='WH')bmp=first?MOOD.DIZZY_A:MOOD.DIZZY_B;
  bitmap(b,0,0,bmp,8,8,old?S.stale:bandColor(lvl));
  const s=printable(last.sgv), hueOff=Math.trunc(t/25)%256;
  const tw=textW(AW,s)-1;
  let x=8+Math.trunc((27-8-tw)/2);
  for(let i=0;i<s.length;i++){
    const ch=s[i];
    drawText(b,AW,ch,x,6,0,old?S.stale:hsv((hueOff+i*35)&255));
    x+=textW(AW,ch);
  }
  trendArrow(b,last,27,1,old);
}},
{name:"Battery and uptime",draw(b,t){
  const pct=S.battery;
  const col=pct<10?C.RED:pct<30?C.YELLOW:C.GREEN;
  const filled=pct===100?10:Math.trunc(pct/10);
  for(let i=0;i<10;i++)px(b,11+i,0,i<filled?col:C.GRAY);
  drawText(b,AW,pct+"%",0,7,0,col);
  const mins=Math.trunc(t/60000);
  let up;
  if(mins<100)up=mins+"M";
  else if(Math.trunc(mins/60)<100)up=Math.trunc(mins/60)+"H";
  else up=Math.trunc(mins/1440)+"D";
  drawText(b,AW,up,31,7,1,C.WHITE);
}},
];
"""

HTML_HEAD = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Nightscout Clock &mdash; Face Simulator</title>
<style>
  :root{color-scheme:dark}
  body{background:#0b0d10;color:#e8eaed;font-family:system-ui,-apple-system,sans-serif;margin:0;padding:16px}
  h1{font-size:20px;margin:0 0 4px}
  .sub{color:#9aa0a6;font-size:13px;margin-bottom:14px}
  .controls{display:flex;flex-wrap:wrap;gap:14px 22px;background:#15181d;border:1px solid #2a2f36;
    border-radius:10px;padding:12px 16px;margin-bottom:16px;align-items:center}
  .ctl{display:flex;align-items:center;gap:8px;font-size:13px}
  .ctl label{color:#9aa0a6}
  .ctl output{min-width:64px;font-variant-numeric:tabular-nums}
  input[type=range]{width:130px}
  select,button{background:#22262c;color:#e8eaed;border:1px solid #3a4048;border-radius:6px;padding:4px 8px;font-size:13px}
  .seg{display:flex;border:1px solid #3a4048;border-radius:6px;overflow:hidden}
  .seg button{border:0;border-radius:0;background:transparent;padding:4px 10px;cursor:pointer;color:#9aa0a6}
  .seg button.on{background:#2f6fed;color:#fff}
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(300px,1fr));gap:14px}
  .card{background:#15181d;border:1px solid #2a2f36;border-radius:10px;padding:10px 12px 12px}
  .card h2{font-size:13px;margin:0 0 8px;color:#c9ced4;font-weight:600}
  .card h2 .id{color:#5f6b76;font-weight:400;margin-right:6px}
  canvas{width:100%;image-rendering:pixelated;background:#000;border-radius:6px;display:block}
  .note{margin-top:14px;color:#5f6b76;font-size:12px}
</style>
</head>
<body>
<h1>Nightscout Clock &mdash; Face Simulator</h1>
<div class="sub">Pixel-exact preview of all {FACECOUNT} clock faces, rendered with the firmware&rsquo;s own font bitmaps and drawing logic. Animations run live.</div>
<div class="controls">
  <div class="ctl"><label>Glucose</label><input id="glucose" type="range" min="40" max="400" value="142"><output id="glucoseOut">142</output></div>
  <div class="ctl"><label>Trend</label><select id="trend">
    <option value="DOUBLE_UP">Double up</option><option value="SINGLE_UP">Single up</option>
    <option value="FORTYFIVE_UP">45&deg; up</option><option value="FLAT" selected>Flat</option>
    <option value="FORTYFIVE_DOWN">45&deg; down</option><option value="SINGLE_DOWN">Single down</option>
    <option value="DOUBLE_DOWN">Double down</option><option value="NONE">None</option>
  </select></div>
  <div class="ctl"><label>Data age</label><input id="age" type="range" min="0" max="40" value="3"><output id="ageOut">3 min</output></div>
  <div class="ctl"><label><input id="nodata" type="checkbox"> No data</label></div>
  <div class="ctl"><label>Units</label><div class="seg" id="units"><button data-v="0" class="on">mg/dL</button><button data-v="1">mmol/L</button></div></div>
  <div class="ctl"><label>Clock</label><div class="seg" id="clock"><button data-v="0" class="on">24h</button><button data-v="1">12h</button></div></div>
  <div class="ctl"><label>Stale color</label><input id="stale" type="color" value="#a451a4"></div>
</div>
<div class="grid" id="grid"></div>
<div class="note">Stale threshold: 20 min (matches firmware default). Timer blocks: one per minute of data age, up to 5. Sample history is synthesized: 3 h of 5-minute readings ending at the chosen value and trend.</div>
<script>
"""

HTML_TAIL = r"""
// ================= ui =================
const grid=document.getElementById('grid');
const states=FACES.map(()=>({}));
const canvases=FACES.map((f,i)=>{
  const card=document.createElement('div');card.className='card';
  const h=document.createElement('h2');h.innerHTML=`<span class="id">${i}</span>${f.name}`;
  const cv=document.createElement('canvas');cv.width=32*15;cv.height=8*15;
  card.appendChild(h);card.appendChild(cv);grid.appendChild(card);
  return cv.getContext('2d');
});
const PX=15;
function renderFace(ctx,b){
  ctx.fillStyle='#000';ctx.fillRect(0,0,32*PX,8*PX);
  for(let y=0;y<8;y++)for(let x=0;x<32;x++){
    const c=b[y*32+x];if(!c)continue;
    ctx.fillStyle=css(c);
    ctx.fillRect(x*PX+1,y*PX+1,PX-2,PX-2);
  }
}
const t0=performance.now();
function frame(){
  const t=performance.now()-t0;
  for(let i=0;i<FACES.length;i++){
    const b=new Uint16Array(32*8);
    try{FACES[i].draw(b,t,states[i]);}catch(e){}
    renderFace(canvases[i],b);
  }
  requestAnimationFrame(frame);
}
function hexTo565(h){const r=parseInt(h.slice(1,3),16),g=parseInt(h.slice(3,5),16),bl=parseInt(h.slice(5,7),16);
  return rgb565(r,g,bl);}
const $=id=>document.getElementById(id);
function refresh(){
  S.value=+$('glucose').value;S.trend=$('trend').value;
  S.ageMin=+$('age').value;S.noData=$('nodata').checked;
  S.stale=hexTo565($('stale').value);
  $('glucoseOut').textContent=S.value+(S.mmol?" ("+(S.value/18).toFixed(1)+")":"");
  $('ageOut').textContent=S.ageMin+" min"+(S.ageMin>=20?" (stale)":"");
  buildReadings();
}
document.querySelectorAll('#units button').forEach(btn=>btn.onclick=()=>{
  document.querySelectorAll('#units button').forEach(x=>x.classList.remove('on'));
  btn.classList.add('on');S.mmol=btn.dataset.v==='1';refresh();});
document.querySelectorAll('#clock button').forEach(btn=>btn.onclick=()=>{
  document.querySelectorAll('#clock button').forEach(x=>x.classList.remove('on'));
  btn.classList.add('on');S.h12=btn.dataset.v==='1';refresh();});
['glucose','trend','age','nodata','stale'].forEach(id=>$(id).addEventListener('input',refresh));
refresh();
requestAnimationFrame(frame);
</script>
</body>
</html>
"""

face_count = JS_FACES.count('{name:')
html = HTML_HEAD.replace('{FACECOUNT}', str(face_count)) + JS_DATA + JS_ENGINE + JS_FACES + HTML_TAIL
open(os.path.join(HERE, 'index.html'), 'w').write(html)
print("faces:", face_count, "bytes:", len(html))
