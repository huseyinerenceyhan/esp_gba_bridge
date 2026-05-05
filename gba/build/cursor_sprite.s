
@{{BLOCK(cursor_sprite)

@=======================================================================
@
@	cursor_sprite, 32x8@4, 
@	+ palette 4 entries, not compressed
@	+ 4 tiles not compressed
@	Total size: 8 + 128 = 136
@
@	Time-stamp: 2026-05-05, 17:55:31
@	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
@	( http://www.coranac.com/projects/#grit )
@
@=======================================================================

	.section .rodata
	.align	2
	.global cursor_spriteTiles		@ 128 unsigned chars
	.hidden cursor_spriteTiles
cursor_spriteTiles:
	.word 0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222
	.word 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
	.word 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x22222222,0x22222222
	.word 0x00000222,0x00000222,0x00000222,0x00000222,0x00000222,0x00000222,0x00000222,0x00000222

	.section .rodata
	.align	2
	.global cursor_spritePal		@ 8 unsigned chars
	.hidden cursor_spritePal
cursor_spritePal:
	.hword 0x0000,0x02A0,0x7FFF,0x0000

@}}BLOCK(cursor_sprite)
