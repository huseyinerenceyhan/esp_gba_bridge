
@{{BLOCK(anchor_sprite)

@=======================================================================
@
@	anchor_sprite, 16x8@4, 
@	+ palette 4 entries, not compressed
@	+ 2 tiles not compressed
@	Total size: 8 + 64 = 72
@
@	Time-stamp: 2026-05-05, 17:55:31
@	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
@	( http://www.coranac.com/projects/#grit )
@
@=======================================================================

	.section .rodata
	.align	2
	.global anchor_spriteTiles		@ 64 unsigned chars
	.hidden anchor_spriteTiles
anchor_spriteTiles:
	.word 0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111
	.word 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000

	.section .rodata
	.align	2
	.global anchor_spritePal		@ 8 unsigned chars
	.hidden anchor_spritePal
anchor_spritePal:
	.hword 0x0000,0x701F,0x0000,0x0000

@}}BLOCK(anchor_sprite)
