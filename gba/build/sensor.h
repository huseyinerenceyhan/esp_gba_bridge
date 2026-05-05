
//{{BLOCK(sensor)

//======================================================================
//
//	sensor, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 68 tiles (t|f|p reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 512 + 2176 + 2048 = 4736
//
//	Time-stamp: 2026-05-05, 17:55:31
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_SENSOR_H
#define GRIT_SENSOR_H

#define sensorTilesLen 2176
extern const unsigned int sensorTiles[544];

#define sensorMapLen 2048
extern const unsigned short sensorMap[1024];

#define sensorPalLen 512
extern const unsigned short sensorPal[256];

#endif // GRIT_SENSOR_H

//}}BLOCK(sensor)
