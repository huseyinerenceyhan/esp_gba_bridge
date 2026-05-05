
//{{BLOCK(msg_info)

//======================================================================
//
//	msg_info, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 242 tiles (t|f|p reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 512 + 7744 + 2048 = 10304
//
//	Time-stamp: 2026-05-05, 17:55:31
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_MSG_INFO_H
#define GRIT_MSG_INFO_H

#define msg_infoTilesLen 7744
extern const unsigned int msg_infoTiles[1936];

#define msg_infoMapLen 2048
extern const unsigned short msg_infoMap[1024];

#define msg_infoPalLen 512
extern const unsigned short msg_infoPal[256];

#endif // GRIT_MSG_INFO_H

//}}BLOCK(msg_info)
