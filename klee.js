const util = require('node:util');
let scales=[];
for(let k=0;k<12;k++) {
   for(let j=k;j<12;j++) {
      for(let l=j;l<12;l++) {
         let arr=[... new Set([k,j,l,(k+j)%12,(k+l)%12,(j+l)%12,(k+j+l)%12])];
         arr = arr.sort((a,b)=>a-b);
         if(arr[0]!=0) arr.unshift(0);
         scales.push({scale:arr,semiIn:[k,j,l]});
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
console.log(util.inspect(scales,{ compact: true, depth: 5, breakLength: 80,maxArrayLength:2000 }));



