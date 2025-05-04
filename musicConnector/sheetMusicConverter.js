let pdfFileName = 'Cramer Etude Nr 1'
let pdfFilePath = `./${pdfFileName}.pdf`;
import { promises as fs } from "node:fs";
import { pdf } from "pdf-to-img";

import { Jimp } from "jimp";







async function storeImgAsBase64(_imgBuffer, _name) {
  const image = await Jimp.fromBuffer(_imgBuffer);
  image.greyscale();
  image.rotate(-90);

  let bitMapOut = [];
  let curByte = 0;
  let horizontalPadding = (8 - (image.bitmap.width % 8)) % 8;
  const channels = 4;

  for (let i = 0; i < image.bitmap.data.length; i += channels)
  {
    let x = (i % (channels * image.bitmap.width)) / channels;
    curByte = curByte << 1 | (image.bitmap.data[i] > 127 ? 0 : 1);

    if (x === image.bitmap.width - 1) // Add padding when last pixel is passed
    {
      curByte = curByte << horizontalPadding;
    } 
    if ((x % 8 === (8 - 1) && x !== 0) || x === image.bitmap.width - 1)
    {
      bitMapOut.push(curByte);
      curByte = 0;
    }
  }

  let buffer = Buffer.from(bitMapOut);
  let encoded = buffer.toString('base64');
  fs.writeFile(`${_name}.base64`, encoded);
}

export async function storePDF(_pdfPath, _outPath) {
  let counter = 0;
  const document = await pdf(_pdfPath, { scale: 1.15 });
  for await (const image of document) {
    await storeImgAsBase64(image, `${_outPath}_[${counter}]`);
    // await fs.writeFile(`${pdfFileName}_[${counter}].png`, image);
    counter++;
  }
}


async function main() {
  let counter = 0;
  const document = await pdf(pdfFilePath, { scale: 1.15 });
  for await (const image of document) {
    await storeImgAsBase64(image, `${pdfFileName}_[${counter}]`);
    // await fs.writeFile(`${pdfFileName}_[${counter}].png`, image);
    counter++;
  }
}
