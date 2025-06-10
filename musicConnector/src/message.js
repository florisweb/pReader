const E_InvalidJSON = Symbol('E_InvalidJSON');


export function parseMessage(_string, _client) {
    let data = parseJSON(_string);
    if (data === E_InvalidJSON) return false;
    if (data.id && data.requestId) return new AuthMessage(data, _client);
    if (data.requestId && !data.isResponse) return new RequestMessage(data, _client);
    if (data.requestId && data.isResponse) return new ResponseMessage(data, _client);
    return new PushMessage(data, _client);
}


class Message {
    isMessage = true;
    type;
    data;
    serviceId;
    _client;

    constructor({type, data, serviceId}, _client) {
        this.type = type;
        this.data = data;
        this.serviceId = serviceId;
        this._client = _client;
    }

    stringify() {
        let messsage = {
            data: this.data,
            type: this.type,
            serviceId: this.serviceId
        }
        return JSON.stringify(messsage);
    }
}



class RequestMessage extends Message {
    isRequestMessage = true;
    requestId;
    constructor({type, data, serviceId, requestId}) {
        super(...arguments);
        this.requestId = requestId;
    }
    
    respond(_response) {
        this._client.send({
            isResponse: true,
            requestId: this.requestId,
            response: _response
        });
    }


    stringify() {
        let messsage = {
            data: this.data,
            type: this.type,
            serviceId: this.serviceId,
            requestId: this.requestId,
            isRequestMessage: true,
        }
        return JSON.stringify(messsage);
    }
}

class ResponseMessage extends RequestMessage {
    isResponse = true;
    response;
    constructor({response}) {
        super(...arguments);
        this.response = response;
    }
    stringify() {
        let messsage = {
            response: this.response,
            type: this.type,
            serviceId: this.serviceId,
            requestId: this.requestId,
            isResponse: this.isResponse,
            isRequestMessage: true,
        }
        return JSON.stringify(messsage);
    }
}


class AuthMessage extends RequestMessage {
    isAuthMessage = true;
    id;
    key;
    constructor({id, key}) {
        super(...arguments);
        this.id = id;
        this.key = key;
    }

    stringify() {
        let messsage = {
            id: this.id,
            key: this.key,
            requestId: this.requestId,
            isRequestMessage: true,
        }
        return JSON.stringify(messsage);
    }
}


class PushMessage extends Message {
    
}




function parseJSON(_string) {
    try {
        return JSON.parse(_string);
    } catch (e) {
        return E_InvalidJSON;
    };
}
