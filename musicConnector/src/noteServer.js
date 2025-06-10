import { parse } from 'node-html-parser';
import fs from "fs";
import { Client } from 'basic-ftp';
import { __dirname } from './polyfill.js';
const CachePath = __dirname + '/cache/noteFile';

let Config;
export default class noteManager {
	#connected = false;
	client

	constructor(_config) {
		Config = _config;
		this.client = new Client();
		this.downloadNote();
	}

	sync() {
		return this.downloadNote();
	}

	async downloadNote() {
		try {
	        await this.client.access(Config);

	        let files = await this.client.list(Config.notePath);
	        let noteFile;
	        for (let file of files)
	        {
	        	if (!file.name.includes(Config.noteNamePart)) continue;
	        	noteFile = file;
	        	break;
	        }
	        if (!noteFile) 
        	{
        		this.client.close();
        		return console.log('Error: note could not be found.');
        	}

	        let res = await this.client.downloadTo(CachePath, Config.notePath + '/' + noteFile.name);
	        if (res.code !== 226) 
        	{
        		this.client.close();
        		return console.log('Error: error while downloading note.');
        	}
	        this.onNoteDownloaded();
	    } catch (e) {
	    	console.log('error', e);
	    }
	    this.client.close();
	}
	onNoteDownloaded() {
		let file = fs.readFileSync(CachePath);
		let HTMLTree = parse(file);
		let body = HTMLTree.querySelector('body');
		let lineElements = body.childNodes;

		let sections = [];
		let curSection;
		for (let el of lineElements)
		{
			if (el.rawTagName !== 'p') continue;
			let curEl = el;
			while (curEl.childNodes[0])
			{
				curEl = curEl.childNodes[0];
			}

			let text = curEl?._rawText;
			if (!text) continue;
			if (text.substr(0, 1) !== '[') // Header
			{
				if (curSection) sections.push(curSection);
				curSection = {name: text, content: []};
			} else {
				if (!curSection) curSection = {name: 'pre', content: []};
				curSection.content.push(text);
			}
		}
		if (curSection) sections.push(curSection);
		console.log(sections);
	}
}
