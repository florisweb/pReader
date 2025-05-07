import { WebSocketServer } from 'ws';
import musicInterface from './musicInterface.js';
import { readFile } from './polyfill.js';
import { PReaderClient } from './clients.js';

let Config = await readFile('./config.json');


// MUSIC API
const MusicInterface = new musicInterface(Config.MusicAPI, (_state) => {
	console.log('change', _state);
});

setInterval(() => MusicInterface.sync(), Config.MusicAPI.updateFrequency);


// Web Socket Server
const wss = new WebSocketServer({ port: Config.server.port });
console.log('Runining server on ' + Config.server.port);
wss.on("connection", _conn => new PReaderClient(_conn));
