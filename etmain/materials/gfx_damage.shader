// gfx_damage.shader


gfx/damage/bullet_mrk
{
	nopicmip
	polygonOffset
	{
		map gfx/damage/bullet_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
	{
		map gfx/damage/bullet_mrk.tga
		blendFunc GL_DST_COLOR GL_ONE
		rgbGen vertex
	}
}

gfx/damage/burn_med_mrk
{
	nopicmip
	polygonOffset
	{
		map gfx/damage/burn_med_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}

gfx/damage/ceramic_mrk
{
	nopicmip
	polygonOffset
	{
		map gfx/damage/ceramic_mrk.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

gfx/damage/glass_mrk
{
	nopicmip
	polygonOffset
	{
		map gfx/damage/glass_mrk.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

gfx/damage/hole_lg_mrk
{
	nopicmip
	polygonOffset
	{
		map gfx/damage/hole_lg_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}

gfx/damage/metal_mrk
{
	nopicmip
	polygonOffset
	{
		map gfx/damage/metal_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
	{
		map gfx/damage/metal_mrk.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
	}
}

gfx/damage/wood_mrk
{
	nopicmip
	polygonOffset
	{
		map gfx/damage/wood_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
	{
		map gfx/damage/wood_mrk.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
	}
}
