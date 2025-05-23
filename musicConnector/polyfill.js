
import fs from 'fs';
import { dirname } from 'path';
import { fileURLToPath } from 'url';

export const __dirname = dirname(fileURLToPath(import.meta.url));
const dataStoragePath = __dirname + '/DBData';

let curRequestPromise;
export async function readFile(_path, _isJSON = true) {
	while (curRequestPromise) await curRequestPromise;

	curRequestPromise = new Promise((resolve, error) => {
	    fs.readFile(_path, (err, content) => {
	        if (err) return error(err);
	        let parsedContent = content;
	        if (!_isJSON) return resolve(content);
	        try {
	            parsedContent = JSON.parse(String(content));
	        } catch (e) {console.log('[FileManager]: Invalid json content: (' + _path + ')', e, _path, content)};
	        resolve(parsedContent);
	    });
	});

	return new Promise((resolve, error) => {
	    curRequestPromise.then((_res) => {
	        resolve(_res);
	        curRequestPromise = false;
	    }, (_error) => {
	        error(_error);
	        curRequestPromise = false;
	    });
	});
}
export async function writeFile(_path, _content) { 
	return new Promise((resolve, error) => {
        fs.writeFile(_path, _content, (err) => {
            if (err) return error(err);
            resolve(true);
        });
	});
}

export async function removeFile(_path) { 
	return new Promise((resolve, error) => {
        fs.rm(_path, (err) => {
            if (err) return error(err);
            resolve(true);
        });
	});
}

const E_NotFound = Symbol('404 Not Found');
export async function sendRequest(_url, _postParams = null, _isJSON = true) {
	let headers = new Headers();
	headers.append('pragma', 'no-cache');
	headers.append('cache-control', 'no-cache');
	headers.append('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');

	let response = await fetch(_url, {
    	method: 'POST',
	    headers: headers,
	    body: _postParams,
	});

	if (response.status === 404) return E_NotFound
	let result = await response.text();
	if (!_isJSON) return result;
	let json;
	try {
		json = JSON.parse(result);
	} catch (e) {
		console.log('Error while parsing json', e, result);
		return result; 
	}
	return json;
}


export let newId = () => {return Math.round(Math.random() * 100000000) + "" + Math.round(Math.random() * 100000000);}