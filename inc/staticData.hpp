//  2022 maddsua | https://gitlab.com/maddsua
//	No warranties are given, etc...
// This file is a component of the HAR Extractor

struct response {
	std::string address;
	std::string mimetype;
	std::string encoding;
	std::string content;
};

struct ThreadItem {
	bool success;
	std::thread worker;
};

struct fileExists {
	std::wstring name;
	unsigned int index;
};

struct mimetype {
	std::string type;
	std::string extension;
};

std::vector <std::string> datatypes = {"All","Text","Media","MPEG .TS","Video","Audio","Images","Fonts","Application"};

std::vector <mimetype> fileExtens = {
	
	{"application/epub+zip",".epub"},
	{"application/gzip",".gz"},
	{"application/java-archive",".jar"},
	{"application/json",".json"},
	{"application/ld+json",".jsonld"},
	{"application/msword",".doc"},
	{"application/octet-stream",".bin"},
	{"application/ogg",".ogx"},
	{"application/pdf",".pdf"},
	{"application/rtf",".rtf"},
	{"application/vnd.apple.mpegurl", ".m3u8"},
	{"application/vnd.amazon.ebook",".azw"},
	{"application/vnd.apple.installer+xml",".mpkg"},
	{"application/vnd.mozilla.xul+xml",".xul"},
	{"application/vnd.ms-excel",".xls"},
	{"application/vnd.ms-fontobject",".eot"},
	{"application/vnd.ms-powerpoint",".ppt"},
	{"application/vnd.oasis.opendocument.presentation",".odp"},
	{"application/vnd.oasis.opendocument.spreadsheet",".ods"},
	{"application/vnd.oasis.opendocument.text",".odt"},
	{"application/vnd.openxmlformats-officedocument.presentationml.presentation",".pptx"},
	{"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",".xlsx"},
	{"application/vnd.openxmlformats-officedocument.wordprocessingml.document",".docx"},
	{"application/vnd.rar",".rar"},
	{"application/vnd.visio",".vsd"},
	{"application/x-7z-compressed",".7z"},
	{"application/x-abiword",".abw"},
	{"application/x-bzip",".bz"},
	{"application/x-bzip2",".bz2"},
	{"application/x-csh",".csh"},
	{"application/x-freearc",".arc"},
	{"application/x-httpd-php",".php"},
	{"application/x-sh",".sh"},
	{"application/x-shockwave-flash",".swf"},
	{"application/x-tar",".tar"},
	{"application/xhtml+xml",".xhtml"},
	{"application/xml",".xml"},
	{"application/zip",".zip"},
	{"audio/3gpp",".3gp"},
	{"audio/3gpp2",".3g2"},
	{"audio/aac",".aac"},
	{"audio/m4a", ".m4a"},
	{"audio/midi audio/x-midi",".midi"},
	{"audio/mpeg",".mp3"},
	{"audio/ogg",".oga"},
	{"audio/opus",".opus"},
	{"audio/wav",".wav"},
	{"audio/webm",".weba"},
	{"font/otf",".otf"},
	{"font/ttf",".ttf"},
	{"font/woff",".woff"},
	{"font/woff2",".woff2"},
	{"image/bmp",".bmp"},
	{"image/gif",".gif"},
	{"image/jpeg",".jpg"},
	{"image/png",".png"},
	{"image/svg+xml",".svg"},
	{"image/tiff",".tiff"},
	{"image/vnd.microsoft.icon",".ico"},
	{"image/webp",".webp"},
	{"image/x-icon",".ico"},
	{"text/calendar",".ics"},
	{"text/css",".css"},
	{"text/csv",".csv"},
	{"text/html",".html"},
	{"text/javascript",".js"},
	{"text/plain",".txt"},
	{"text/xml",".xml"},
	{"video/3gpp",".3gp"},
	{"video/3gpp2",".3g2"},
	{"video/mp2t",".ts"},
	{"video/mpeg",".mpeg"},
	{"video/ogg",".ogv"},
	{"video/webm",".webm"},
	{"video/x-msvideo",".avi"},
	{"video/quicktime", ".mov"},
	{"video/mp4", ".mp4"}
};
