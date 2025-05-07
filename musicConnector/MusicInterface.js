import fetch from 'node-fetch';
import { readFile, writeFile, removeFile, sendRequest } from './polyfill.js';
import { readdir } from 'node:fs/promises'

import { storePDFAsBase64Files } from './sheetMusicConverter.js';

const fileCachePath = `./cachedSheetMusic`;
let curMusicItemState = [];
let Config;

class musicInterface {
	#onStateChange;

	get musicItems() {
		return curMusicItemState;
	}

	constructor(_config, _onStateChange) {
		Config = _config;
		this.sync();
		this.#onStateChange = _onStateChange;
	}

	async #fetchMusicItems() {
		let result = await sendRequest(Config.path + '/database/getMusicItems.php', `APIKey=${Config.key}`);
		if (typeof result === 'string') return false;
		if (result.error) return console.log('Error while fetching music items:', result);

		return result.result.filter(item => item.isOwnedByMe && item.sheetMusicSource).map(item => {
			return {title: item.title, status: item.status, sheetMusicSource: item.sheetMusicSource, id: item.id};
		});
	}


	async sync() {
		let items = await this.#fetchMusicItems();
		let newItems = items.filter(item => !curMusicItemState.map(r => r.id).includes(item.id));
		let lostItems = curMusicItemState.filter(item => !items.map(r => r.id).includes(item.id));

		for (let item of lostItems) await onItemLose(item);
		
		let modifiedItem = false;	
		for (let item of items) // Shared items
		{
			if (await onItemPotentialChange(item)) modifiedItem = true;
		}

		for (let item of newItems) await onItemAdd(item);

		// console.log('changes', 'new', newItems.map(r => r.title), 'lost', lostItems.map(r => r.title), 'prev', curMusicItemState.map(r => r.title), 'in', items.map(r => r.title));

		if (!modifiedItem && newItems.length === 0 && lostItems.length === 0) return;
		this.#onStateChange(curMusicItemState.filter(r => typeof r.pageCount === 'number')); // Only show items that have their pages downloaded
	}

	async getMusicImage(_musicId, _pageIndex) {
		return new Promise((resolve) => {
			let path = `${fileCachePath}/${_musicId}_[${_pageIndex}].base64`;
			
			readFile(path, false).then((_file) => resolve(_file), (_e) => {
				console.log('error while reading', path, _e);
				resolve(false);
			});
		});
	}
}

export default musicInterface;



async function onItemPotentialChange(_newItem) {
	let oldItem = curMusicItemState.find(i => i.id === _newItem.id);
	if (!oldItem) return; // Item did not change, but was added: will be handled by onItemAdd
	if (oldItem.sheetMusicSource === _newItem.sheetMusicSource)
	{
		if (oldItem.title === _newItem.title && oldItem.status === _newItem.status) return;
		curMusicItemState[curMusicItemState.findIndex(i => i.id === _newItem.id)] = _newItem;
	} else {
		// Remove and re-download
		await onItemLose(_newItem);
		await onItemAdd(_newItem);
	}
	return true;
}

async function onItemLose(_item) {
	curMusicItemState = curMusicItemState.filter(r => r.id !== _item.id);
	await removeFilesByMusicId(_item.id);
}

async function onItemAdd(_item) {
	curMusicItemState.push(_item);
	_item.pageCount = await downloadSheetMusic(_item.id)
}

async function downloadSheetMusic(_musicItemId) {
	let headers = new Headers();
	headers.append('pragma', 'no-cache');
	headers.append('cache-control', 'no-cache');
	headers.append('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');

	let response = await fetch(Config.path + '/database/getMusicFile.php?type=sheetMusic&id=' + _musicItemId, {
    	method: 'POST',
	    headers: headers,
	    body: `APIKey=${Config.key}`,
	});

	let buffer = await response.buffer();
	try {
		let path = `${fileCachePath}/${_musicItemId}`;
		await writeFile(path + '.pdf', buffer);
		return await storePDFAsBase64Files(buffer, path);
	} catch (e) {
		console.log('error', e)
	}
}

async function removeFilesByMusicId(_id) {
	let files = await readdir(fileCachePath);
	for (let file of files)
	{
		if (!file.includes(_id + '_')) continue;
		removeFile(fileCachePath + '/' + file);
	}
}
