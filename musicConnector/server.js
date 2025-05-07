import SocketServer from './webSocketServer.js';

import musicInterface from './musicInterface.js';
import { readFile } from './polyfill.js';

let Config = await readFile('./config.json');


SocketServer.setup();

// MUSIC API
const MusicInterface = new musicInterface(Config.MusicAPI, (_state) => {
	console.log('change', _state);
	SocketServer.onMusicStateChange(_state);
});


setInterval(() => MusicInterface.sync(), Config.MusicAPI.updateFrequency);
