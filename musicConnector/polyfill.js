
import fs from 'fs';
import { dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = getCurDir();
const dataStoragePath = __dirname + '/DBData';

export function getCurDir() {
    return dirname(fileURLToPath(import.meta.url));
}

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