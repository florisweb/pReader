#!/usr/bin/env node
import { promises as fs } from "node:fs";
import { pdf } from "pdf-to-img";
import { Jimp } from "jimp";


async function storeImgAsBase64(_imgBuffer, _name) {
  const image = await Jimp.fromBuffer(_imgBuffer);
  image.greyscale();

  let bitMap = colorImageToBW(image);
  let targetWidth = 960;

  let bitMapOut = scaleBitMapToSize(bitMap, Math.ceil(image.width/8)*8, 8);
  let buffer = Buffer.from(bitMapOut);
  let encoded = buffer.toString('base64');
  fs.writeFile(`${_name}.base64`, encoded);
}

async function storeImgAsHex(_imgBuffer, _name) {
  const image = await Jimp.fromBuffer(_imgBuffer);
  image.greyscale();
  image.rotate(-90);

  let bitMap = colorImageToBW(image, true);
  let hex = bitMap.map(b => '0x' + (b.toString(16).length == 1 ? '0' + b.toString(16) : b.toString(16))).join(',');
  console.log('Hex:', hex);
  fs.writeFile(`${_name}.hex`, hex);
}



function scaleBitMapToSize(_bitmap, _originalWidth, _newWidth) {
  const scalar = _newWidth / _originalWidth;
  const pxSize = 1 / scalar; // How many old pixels a new pixel takes
  let pxOut = [];
  let newHeight = _bitmap.length * 8 / _originalWidth * scalar;
  newHeight = 1;

  for (let x = 0; x < _newWidth; x++)
  {
    let originalX = x / scalar;
    let firstX = Math.floor(originalX);
    let firstXPxPerc = Math.min(1 - (originalX - firstX), pxSize);
    let lastX = Math.floor(originalX + pxSize);
    let fullPxWidth = Math.floor(pxSize - firstXPxPerc);
    let lastXPxPerc = (pxSize - firstXPxPerc - fullPxWidth);

    let summed = 0;

    // for (let h = 0; h < fullPxWidth; h++)
    // {
      summed += getPixelFromBitmap(_bitmap, _originalWidth, firstX, 0) * firstXPxPerc;
      
      summed += getPixelFromBitmap(_bitmap, _originalWidth, lastX, 0) * lastXPxPerc;

      for (let w = 0; w < fullPxWidth; w++)
      {
        let orgX = firstX + w;
        summed += getPixelFromBitmap(_bitmap, _originalWidth, orgX, 0);
      }
    // }

    summed /= pxSize;// * pxSize;
    pxOut.push(summed > .5 ? 1 : 0);
    console.log(x, firstXPxPerc, fullPxWidth, lastXPxPerc, summed);
  }


  // Convert px's to bytes
  let bitMapOut = [];
  for (let byte = 0; byte < _newWidth / 8; byte++)
  {
    let dataByte = 0;
    for (let bit = 0; bit < 8; bit++)
    {
      dataByte = (dataByte << 1) | pxOut[byte * 8 + bit];
    }
    bitMapOut.push(dataByte);
  }

  return bitMapOut;
}

function getPixelFromBitmap(_bitmap, _originalWidth, _x, _y) {
  let pxIndex = _x + _y * _originalWidth;
  let byteIndex = Math.floor(pxIndex / 8);
  let bitIndex = pxIndex  - byteIndex * 8;
  let byte = _bitmap[byteIndex];
  return byte >> (7 - bitIndex) & 1;
}


function colorImageToBW(_image, _invert = false) {
  let bitMapOut = [];
  let curByte = 0;
  let horizontalPadding = (8 - (_image.bitmap.width % 8)) % 8;
  const channels = 4;

  for (let i = 0; i < _image.bitmap.data.length; i += channels)
  {
    let x = (i % (channels * _image.bitmap.width)) / channels;
    curByte = curByte << 1 | (_image.bitmap.data[i] > 127 ? (_invert ? 1 : 0) : (_invert ? 0 : 1));

    if (x === _image.bitmap.width - 1) // Add padding when last pixel is passed
    {
      curByte = curByte << horizontalPadding;
    } 
    if ((x % 8 === (8 - 1) && x !== 0) || x === _image.bitmap.width - 1)
    {
      bitMapOut.push(curByte);
      curByte = 0;
    }
  }
  return bitMapOut;
}




async function handlePDF(_path, _fileName, _asHex) {
  let counter = 0;
  const document = await pdf(_path, { scale: 1.15 });
  for await (const image of document) {
    await storeImgAsBase64(image, `${_fileName}_[${counter}]`);
    counter++;
  }
}

async function handleImg(_path, _fileName, _asHex) {
  const image = await Jimp.read(_path);
  if (_asHex) return await storeImgAsHex(await image.getBuffer('image/bmp'), _fileName);
  await storeImgAsBase64(await image.getBuffer('image/bmp'), _fileName);
}


let args = process.argv;
if (args.length <= 2)
{
  console.log(`Usage:
    node converter.js [filename] [output]
    - filename: *.pdf, *.png
    - output: base64 [default], hex
`);
} else {
  let path = args[2];

  let parts = path.split('.pdf');
  let fileName = parts[0].split('/').pop().split('.png')[0];
  let isPDF = parts.length > 1 && parts[1] === "";
  let asHex = args[3] === "hex";

  if (isPDF) {
    await handlePDF(path, fileName, asHex);
  } else {
    await handleImg(path, fileName, asHex);
  }
}

