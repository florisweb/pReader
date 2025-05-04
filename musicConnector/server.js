import fetch from 'node-fetch';
import { readFile, writeFile } from './polyfill.js';
import { storePDF } from './sheetMusicConverter.js';

let Config = await readFile('./config.json');
const E_NotFound = Symbol('404 Not Found');

async function fetchMusicItems() {
	let result = await sendRequest(Config.MusicPath + '/database/getMusicItems.php', `APIKey=${Config.MusicAPIKey}`);
	if (typeof result === 'string') return false;
	if (result.error) return console.log('Error while fetching music items:', result);

	return result.result.filter(item => item.isOwnedByMe && item.sheetMusicSource).map(item => {
		return {title: item.title, status: item.status, sheetMusicSource: item.sheetMusicSource, id: item.id};
	});
}

async function sendRequest(_url, _postParams = null, _isJSON = true) {
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

let curMusicItemState = [];

async function sync() {
	let items = await fetchMusicItems();
	let newItems = items.filter(item => !curMusicItemState.map(r => r.id).includes(item.id));
	let lostItems = curMusicItemState.filter(item => !items.map(r => r.id).includes(item.id));

	for (let item of lostItems) await onItemLose(item);
	
	for (let item of curMusicItemState) // Shared items
	{
		

	}

	for (let item of newItems) await onItemAdd(item);

	console.log('changes', 'new', newItems.map(r => r.title), 'lost', lostItems.map(r => r.title), 'prev', curMusicItemState.map(r => r.title), 'in', items.map(r => r.title));

	setTimeout(sync, Config.updateFrequency)
}
sync();

function onItemLose(_item) {
	curMusicItemState = curMusicItemState.filter(r => r.id !== _item.id);

}
async function onItemAdd(_item) {
	curMusicItemState.push(_item);
	console.log('download');
	await downloadSheetMusic(_item.id)
}



const fileCachePath = `./cachedSheetMusic`;
async function downloadSheetMusic(_musicItemId) {
	let headers = new Headers();
	headers.append('pragma', 'no-cache');
	headers.append('cache-control', 'no-cache');
	headers.append('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');

	let response = await fetch(Config.MusicPath + '/database/getMusicFile.php?type=sheetMusic&id=' + _musicItemId, {
    	method: 'POST',
	    headers: headers,
	    body: `APIKey=${Config.MusicAPIKey}`,
	});
	let buffer = await response.buffer();
	// let text = await response.text();
	// console.log(text);
	// console.log(Config.MusicPath + '/database/getMusicFile.php?type=sheetMusic&id=' + _musicItemId);
	// let result = await sendRequest(Config.MusicPath + '/database/getMusicFile.php?type=sheetMusic&id=' + _musicItemId, `APIKey=${Config.MusicAPIKey}`, false);
	// if (result?.error) return console.log('Error while downloading:', result);
	// if (result === E_NotFound) return console.log('Error while downloading: 404');
	try {
		let path = `${fileCachePath}/${_musicItemId}`;
		await writeFile(path + '.pdf', buffer);
		await storePDF(buffer, path);
	} catch (e) {
		console.log('error', e)
	}
	
}



// const fileCachePath = `./cachedSheetMusic`;
// async function downloadSheetMusic(_musicItemId) {




// 	console.log(Config.MusicPath + '/database/getMusicFile.php?type=sheetMusic&id=' + _musicItemId);
// 	let result = await sendRequest(Config.MusicPath + '/database/getMusicFile.php?type=sheetMusic&id=' + _musicItemId, `APIKey=${Config.MusicAPIKey}`, false);
// 	if (result?.error) return console.log('Error while downloading:', result);
// 	if (result === E_NotFound) return console.log('Error while downloading: 404');

// 	let path = `${fileCachePath}/${_musicItemId}`;
// 	await writeFile(path + '.pdf', result);
// 	await storePDF(path + '.pdf', path);
// }

