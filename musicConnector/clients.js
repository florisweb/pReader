import { parseMessage } from './message.js';
import { readFile, newId, __dirname } from './polyfill.js';
import SocketServer from './webSocketServer.js';
let Config = await readFile(__dirname + '/config.json');




export let clients = [];
// Remove disconnected clients
const interval = setInterval(function () {
  clients.forEach(function (client) {
    if (client.isAlive === false) return client.conn.terminate();
    client.isAlive = false;
    client.conn.ping();
    client.send({type: "heartbeat"});
  });
}, 10000);



class BaseClient {
    id              = newId();
    conn;
    isAlive         = true;
    isDead          = false;

    constructor(_conn) {
        this.conn = _conn;
        clients.push(this);

        this.conn.on('pong',    () => {if (!this.isDead) this._onPong()});
        this.conn.on('close',   () => {if (!this.isDead) this._onClose()});
        this.conn.on('error',   () => {if (!this.isDead) this._onClose()});
        this.conn.on('close',   () => {if (!this.isDead) this._onClose()});
        this.conn.on('end',     () => {if (!this.isDead) this._onClose()});
        this.conn.on('message', (_buffer) => {if (!this.isDead) this._onMessage(_buffer)});
    }

    _onPong() {
        this.isAlive = true;
    }
    _onClose() {
        clients = clients.filter((client) => client.id != this.id);
        this.isDead = true;
    }

    _onMessage(_buffer) { 
        let message = parseMessage(_buffer, this); 
        if (message === false) return this.send({error: "Invalid request"});
        return message;
    }

    send(_obj) {
        if (typeof _obj === 'string') return this.conn.send(_obj);
        if (_obj.isMessage) return this.conn.send(_obj.stringify());
        this.conn.send(JSON.stringify(_obj));
    }
}


export class PReaderClient extends BaseClient {
    #authenticated = false;

    

    constructor(_conn) {
        super(_conn);
    }

    _onMessage(_buffer) {
        let message = super._onMessage(_buffer);
        if (!message) return;
        
        if (this.#authenticated) return this.onMessage(message);
        if (!message.isRequestMessage) return this.send({error: "Parameters missing"});
        if (!message.isAuthMessage) return message.respond({error: "Parameters missing"});

        if (message.id !== Config.server.serviceId)
        {
            console.log('wrong service!');
            return message.respond({error: "Service not found"});
        }
        if (message.key !== Config.server.serviceKey) {
            console.log('wrong key!');
            return message.respond({error: "Invalid Key"});
        }
        
        this.#authenticated = true;
        console.log('Authenticated!');
        message.respond({type: "auth", data: true});

        SocketServer.pushCurState(this);
    }


    onMessage(_message) {
        console.log('mes', _message.type, _message.data);
        switch (_message.type)
        {
            case "requestMusicImage": 
                return SocketServer.handleRequestMusicImage(_message);
            case "getImageSection":
                if (_message.isRequestMessage) return SocketServer.handleRequestImageSection(_message);
            case "getThumbnail":
                if (_message.isRequestMessage) return SocketServer.handleRequestThumbnail(_message);
        }
    }
}