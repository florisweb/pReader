
import { promises as fs } from "node:fs";
import { pdf } from "pdf-to-img";
import { Jimp } from "jimp";

export async function storePDFAsBase64Files(_pdfPath, _outPath) {
  let counter = 0;
  const document = await pdf(_pdfPath, { scale: 1.15 });
  let thumbnailImg;
  for await (const image of document) {
    await storeImgAsBase64(image, `${_outPath}_[${counter}]`, false);
    if (!thumbnailImg) thumbnailImg = image;
    await fs.writeFile(`${_outPath}_[${counter}]`, image);
    counter++;
  }

  storeImgAsBase64(thumbnailImg, `${_outPath}_[THUMB]`, true);
  return counter;
}

async function storeImgAsBase64(_imgBuffer, _name, _isThumbnail = false) {
  const targetWidth = _isThumbnail ? 184 : 968;
  const targetHeight = _isThumbnail ? 130 : 684; //684
  const pixelCutOffPoint = _isThumbnail ? 200 : 190;
  const image = await Jimp.fromBuffer(_imgBuffer);
  image.greyscale();
  image.rotate(-90);

  let preWidth = image.bitmap.width;
  image.scale(targetWidth / image.bitmap.width);

  let bitMapOut = [];
  let curByte = 0;
  let horizontalPadding = (8 - (image.bitmap.width % 8)) % 8;
  const channels = 4;


  let requiredPixels = targetHeight * targetWidth;
  let excessPixels = requiredPixels - image.bitmap.data.length / channels;
  console.log('excess', excessPixels);
  for (let i = -Math.floor(excessPixels / 2); i < requiredPixels * channels - Math.floor(excessPixels / 2); i += channels)
  {
    if (i < 0 || i > image.bitmap.data.length) // Pad 'bottom' if image not 'high' enough
    {
      bitMapOut.push(0);
      continue;
    }
    
    let x = (i % (channels * image.bitmap.width)) / channels;
    curByte = curByte << 1 | (image.bitmap.data[i] > pixelCutOffPoint ? 0 : 1);

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
  // image.bitmap.data = buffer;
  // image.bitmap.width = targetWidth;
  // image.bitmap.height = targetHeight;
  // image.write(_name + '.png');

  let encoded = buffer.toString('base64');
  fs.writeFile(`${_name}.base64`, encoded);
}