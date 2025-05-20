import SocketServer from './webSocketServer.js';
import noteServer from './noteServer.js';
import musicInterface from './musicInterface.js';
import { readFile } from './polyfill.js';

let Config = await readFile('./config.json');
const NoteServer = new noteServer(Config.NotesAPI);
setInterval(() => NoteServer.sync(), Config.NotesAPI.updateFrequency);

// // MUSIC API
const MusicInterface = new musicInterface(Config.MusicAPI, (_state) => {
	console.log('change', _state);
	SocketServer.onMusicStateChange(_state);
});

SocketServer.setup(MusicInterface);

setInterval(() => MusicInterface.sync(), Config.MusicAPI.updateFrequency);
