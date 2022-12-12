const util = require('node:util');
function rot(arr,idx) {
  if(idx==0) return arr;
  let len=arr.length;
  let d=arr[idx];
  let ret=[0];
  for(let k=idx+1;k<len;k++) {
    let a=(arr[k]-d+12)%12;
    ret.push(a);
  }
  for(let k=0;k<idx-1;k++) {
    let a=(arr[k]-d+12)%12;
    ret.push(a);
  }
  return ret;
}
let scales=[];
for(let k=0;k<12;k++) {
   for(let j=k;j<12;j++) {
      for(let l=j;l<12;l++) {
         let arr=[... new Set([k,j,l,(k+j)%12,(k+l)%12,(j+l)%12,(k+j+l)%12])];
         arr = arr.sort((a,b)=>a-b);
         if(arr[0]!=0) arr.unshift(0);
         for(let n=0;n<arr.length;n++) {
           scales.push({scale:rot(arr,n),semiIn:[k,j,l],ofs:arr[n]});
         }
         arr=[... new Set([k,j,12-l,(k+j)%12,(k-l+12)%12,(j-l+12)%12,(k+j-l+12)%12])];
         arr = arr.sort((a,b)=>a-b);
         if(arr[0]!=0) arr.unshift(0);
         for(let n=0;n<arr.length;n++) {
           scales.push({scale:rot(arr,n),semiIn:[k,j,-l],ofs:arr[n]});
         }
         arr=[... new Set([k,12-j,12-l,(k-j+12)%12,(k-l+12)%12,(-j-l+24)%12,(k-j-l+12)%12])];
         arr = arr.sort((a,b)=>a-b);
         if(arr[0]!=0) arr.unshift(0);
         for(let n=0;n<arr.length;n++) {
           scales.push({scale:rot(arr,n),semiIn:[k,-j,-l],ofs:arr[n]});
         }
         arr=[... new Set([12-k,j,l,(12-k+j)%12,(12-k+l)%12,(j+l)%12,(12-k+j+l)%12])];
         arr = arr.sort((a,b)=>a-b);
         if(arr[0]!=0) arr.unshift(0);
         for(let n=0;n<arr.length;n++) {
           scales.push({scale:rot(arr,n),semiIn:[-k,j,l],ofs:arr[n]});
         }
         arr=[... new Set([12-k,12-j,l,(24-k-j)%12,(12-k+l)%12,(12-j+l)%12,(24-k-j+l)%12])];
         arr = arr.sort((a,b)=>a-b);
         if(arr[0]!=0) arr.unshift(0);
         for(let n=0;n<arr.length;n++) {
           scales.push({scale:rot(arr,n),semiIn:[-k,-j,l],ofs:arr[n]});
         }
         arr=[... new Set([12-k,j,12-l,(12-k+j)%12,(24-k-l)%12,(j+12-l)%12,(24-k+j-l)%12])];
         arr = arr.sort((a,b)=>a-b);
         if(arr[0]!=0) arr.unshift(0);
         for(let n=0;n<arr.length;n++) {
           scales.push({scale:rot(arr,n),semiIn:[-k,j,-l],ofs:arr[n]});
         }
     }
   }
}
scales = scales.sort( (a,b) => {
  let lenA = a.scale.length;
  let lenB = b.scale.length;
  if(lenA==lenB) {
    for(let k=0;k<lenA;k++) {
      if(a.scale[k]!=b.scale[k]) return a.scale[k]-b.scale[k];
    }
    return 0;
  } else {
    return lenA-lenB;
  }
});
scales.forEach( scl => {
  let str=scl.scale.join('_');
  str+= ' ' + scl.semiIn.join('_');
  str+= ' ' + scl.ofs;
  console.log(str); 
});
//console.log(util.inspect(scales,{ compact: true, depth: 5, breakLength: 80,maxArrayLength:2000 }));



