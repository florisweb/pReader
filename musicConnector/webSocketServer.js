import { WebSocketServer } from 'ws';
import { PReaderClient, clients } from './clients.js';
import { readFile } from './polyfill.js';

let Config = await readFile('./config.json');


const SocketServer = new class {
	#wss;
	constructor() {

	}

	setup() {
		this.#wss = new WebSocketServer({ port: Config.server.port });
		console.log('Runining server on ' + Config.server.port);
		this.#wss.on("connection", _conn => new PReaderClient(_conn));
	}


	#curMusicState = [];
	onMusicStateChange(_newState) {
		this.#curMusicState = _newState;
		this.pushCurState();
	}

	pushCurState(_client) {
		let availableMusic = this.#curMusicState.map(r => {
			return {
				pages: r.pageCount,
				learningState: statusToLearningState(r.status),
				name: r.title,
				id: r.id
			}
		});
		
		if (_client) 
		{
			return _client.send({
				type: "curState",
				data: {
					availableMusic: availableMusic
				}
			});
		}

		for (let client of clients)
		{
			client.send({
				type: "curState",
				data: {
					availableMusic: availableMusic
				}
			})
		}
	}
}

function statusToLearningState(_status) {
	switch (_status) {
		case "learning": return 0;
		case "wishList": return 1;
		default: return 2;
	}
}



export default SocketServer;