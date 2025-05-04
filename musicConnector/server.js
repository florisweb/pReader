
import musicInterface from './musicInterface.js';
import { readFile } from './polyfill.js';

let Config = await readFile('./config.json');
const MusicInterface = new musicInterface(Config, (_state) => {
	console.log('change', _state);
});

setInterval(() => MusicInterface.sync(), Config.updateFrequency);