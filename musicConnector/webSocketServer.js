import { WebSocketServer } from 'ws';
import { PReaderClient, clients } from './clients.js';
import { readFile, newId } from './polyfill.js';

let Config = await readFile('./config.json');

let MusicInterface;
const SocketServer = new class {
	#wss;
	#requestTimeoutDuration = 1000 * 60 * 5;
    #curRequestedMusicImages = [];

	constructor() {

	}

	setup(_musicInterface) {
		MusicInterface = _musicInterface;
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

		let message = {
			type: "curState",
			data: {
				availableMusic: availableMusic
			}
		}
		
		if (_client) return _client.send(message);
		for (let client of clients) client.send(message);
	}



	 async handleRequestMusicImage(_message) {
        // Remove timed out requests
        this.#curRequestedMusicImages = this.#curRequestedMusicImages.filter(r => new Date() - r.startTime < this.#requestTimeoutDuration);

        let data = await MusicInterface.getMusicImage(_message.data.musicId, _message.data.pageIndex);
        if (!data) return _message.respond({error: 'E_PageNotFound'});

        let request = {
            musicName: _message.data.musicName,
            pageIndex: _message.data.pageIndex,
            dataBuffer: data,
            startTime: new Date(),
            id: parseInt(newId().substr(0, 5))
        }
        this.#curRequestedMusicImages.push(request);

        _message.respond({
            dataLength: data.length,
            imageWidth: 968,
            imageRequestId: request.id
        });
    }


    handleRequestImageSection(_message) {
        let request = this.#curRequestedMusicImages.find((req) => req.id === _message.data.imageRequestId);
        if (!request) return _message.respond({error: 'E_requestNotFound'});

        let startPos = _message.data.startIndex;
        let sectionLength = _message.data.sectionLength;
                
        if (startPos + sectionLength >= request.dataBuffer.length) 
        {
            this.#curRequestedMusicImages = this.#curRequestedMusicImages.filter(r => r.id !== _message.data.imageRequestId);
            console.log('all image data has been requested');
        }

        let responseString = request.dataBuffer.slice(startPos, startPos + sectionLength).toString('base64');
        console.log('getImageSection: respond with:', responseString.substr(0, 25) + '... (' + responseString.length + ')');
        _message.respond({
            startIndex: startPos,
            data: responseString
        });
    }
}

function statusToLearningState(_status) {
	switch (_status) {
		case "learning": return 0;
		case "wishList": return 1;
		case "onHold": return 2;
		case "finished": return 3;
		default: return 4;
	}
}



export default SocketServer;