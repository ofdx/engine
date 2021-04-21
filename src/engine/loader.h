/*
	FileLoader
	mperron (2019)

	A class which models game assets. These are instantiated via the
	automatically generated assetblob file, with base64 encoded asset
	data.
*/
#include "base64.h"

string get_save_path();

class FileLoader {
	size_t size_raw;
	string fname;
	const char *data_raw;
	string data;

	static map<string, FileLoader*> assets;

	SDL_RWops *rw = NULL;
	SDL_Surface *sf = NULL;
	Mix_Music *mu = NULL;
	Mix_Chunk *snd = NULL;
public:
	FileLoader(size_t size_raw, string fname, const char *data){
		this->size_raw = size_raw;
		this->fname = fname;
		this->data = string(data);
		this->data_raw = data;
	}

	// Get a surface for this asset if it's an image.
	SDL_Surface *surface(){
		if(!sf)
			sf = SDL_LoadBMP_RW(rwops(), 0);

		return this->sf;
	}

	SDL_RWops *rwops(){
		if(!rw)
			rw = SDL_RWFromMem(((void*) data_raw), size_raw);

		return rw;
	}

	Mix_Music *music(){
		if(!mu)
			mu = Mix_LoadMUS_RW(rwops(), 0);

		return mu;
	}

	Mix_Chunk *sound(){
		if(!snd)
			snd = Mix_LoadWAV_RW(rwops(), 0);

		return snd;
	}

	const char *text(){
		return data_raw;
	}

	void write_to_disk(){
		std::filesystem::path path_out(get_save_path() + fname);

		// Create the output directory structure.
		try {
			std::filesystem::create_directories(path_out.parent_path());
		} catch(...){
			cerr << "Cannot write to preferences directory." << endl;
			return;
		}

		FILE *outfile = fopen(path_out.make_preferred().string().c_str(), "w");
		if(outfile){
			fwrite(data_raw, sizeof(char), size_raw, outfile);
			fclose(outfile);
		} else {
			cerr << "Failed to cache resource to disk. (" << fname << ")" << endl;
		}
	}

	static void load(string fname, FileLoader *fl);
	static void decode_all();
	static FileLoader *get(string);
};

// A map of all the assets.
map<string, FileLoader*> FileLoader::assets;

// Called by the assetblob code to create file data.
void FileLoader::load(string fname, FileLoader *fl){
	assets[fname] = fl;
}

// Find a file by path.
FileLoader *FileLoader::get(string fname){
	FileLoader *fl = assets[fname];

	// File not loaded or built in. Check disk.
	if(!fl){
		std::filesystem::path path_in(get_save_path() + fname);
		FILE *infile = fopen(path_in.string().c_str(), "r");

		if(infile){
			// Get file size.
			fseek(infile, 0L, SEEK_END);
			size_t fsize = ftell(infile);
			rewind(infile);

			char *data = (char*) calloc(fsize, sizeof(char));
			fread(data, sizeof(char), fsize, infile);
			fclose(infile);

			fl = new FileLoader(fsize, fname, data);
			assets[fname] = fl;
		}
	}

	if(!fl){
		cerr << "File not found: " << fname << endl;
	}

	return fl;
}

// Turn the base64 encoded data into real data.
void FileLoader::decode_all(){
	for(auto x : assets){
		FileLoader *fl = x.second;

		fl->data_raw = base64_dec(fl->data.c_str(), strlen(fl->data.c_str()));
	}
}
