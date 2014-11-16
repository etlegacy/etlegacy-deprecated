/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file renderer2/tr_shader.c
 * @brief Parsing and definition of shaders
 */

#include "tr_local.h"

#define MAX_GUIDETEXT_HASH      2048
static char **guideTextHashTable[MAX_GUIDETEXT_HASH];

#define MAX_SHADERTABLE_HASH    1024
static shaderTable_t *shaderTableHashTable[MAX_SHADERTABLE_HASH];

#define FILE_HASH_SIZE          1024
static shader_t *shaderHashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH     2048
static char **shaderTextHashTable[MAX_SHADERTEXT_HASH];

#define generateHashValue(fname, size) Q_GenerateHashValue(fname, size, qfalse, qtrue)

static char *s_guideText;
static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderTable_t table;
static shaderStage_t stages[MAX_SHADER_STAGES];
static shader_t      shader;
static texModInfo_t  texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
static qboolean      deferLoad;

// these are here because they are only referenced while parsing a shader
static char       implicitMap[MAX_QPATH];
static unsigned   implicitStateBits;
static cullType_t implicitCullType;

void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset)
{
	char      strippedName[MAX_QPATH];
	int       hash;
	shader_t  *sh, *sh2;
	qhandle_t h;

	sh = R_FindShaderByName(shaderName);
	if (sh == NULL || sh == tr.defaultShader)
	{
		h  = RE_RegisterShader(shaderName);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader)
	{
		Ren_Warning("WARNING: R_RemapShader: shader %s not found\n", shaderName);
		return;
	}

	sh2 = R_FindShaderByName(newShaderName);
	if (sh2 == NULL || sh2 == tr.defaultShader)
	{
		h   = RE_RegisterShader(newShaderName);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader)
	{
		Ren_Warning("WARNING: R_RemapShader: new shader %s not found\n", newShaderName);
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension(shaderName, strippedName, sizeof(strippedName));
	hash = generateHashValue(strippedName, FILE_HASH_SIZE);
	for (sh = shaderHashTable[hash]; sh; sh = sh->next)
	{
		if (Q_stricmp(sh->name, strippedName) == 0)
		{
			if (sh != sh2)
			{
				sh->remappedShader = sh2;
			}
			else
			{
				sh->remappedShader = NULL;
			}
		}
	}
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector(char **text, int count, float *v)
{
	char *token;
	int  i;

	token = COM_ParseExt2(text, qfalse);
	if (strcmp(token, "("))
	{
		Ren_Warning("WARNING: missing parenthesis '(' in shader '%s' of token '%s'\n", shader.name, token);
		return qfalse;
	}

	for (i = 0; i < count; i++)
	{
		token = COM_ParseExt2(text, qfalse);
		if (!token[0])
		{
			Ren_Warning("WARNING: missing vector element in shader '%s' - no token\n", shader.name);
			return qfalse;
		}
		v[i] = atof(token);
	}

	token = COM_ParseExt2(text, qfalse);
	if (strcmp(token, ")"))
	{
		Ren_Warning("WARNING: missing parenthesis ')' in shader '%s' of token '%s'\n", shader.name, token);
		return qfalse;
	}

	return qtrue;
}

const opstring_t opStrings[] =
{
	{ "bad",                OP_BAD                },
	{ "&&",                 OP_LAND               },
	{ "||",                 OP_LOR                },
	{ ">=",                 OP_GE                 },
	{ "<=",                 OP_LE                 },
	{ "==",                 OP_LEQ                },
	{ "!=",                 OP_LNE                },
	{ "+",                  OP_ADD                },
	{ "-",                  OP_SUB                },
	{ "/",                  OP_DIV                },
	{ "%",                  OP_MOD                },
	{ "*",                  OP_MUL                },
	{ "neg",                OP_NEG                },
	{ "<",                  OP_LT                 },
	{ ">",                  OP_GT                 },
	{ "(",                  OP_LPAREN             },
	{ ")",                  OP_RPAREN             },
	{ "[",                  OP_LBRACKET           },
	{ "]",                  OP_RBRACKET           },
	{ "c",                  OP_NUM                },
	{ "time",               OP_TIME               },
	{ "parm0",              OP_PARM0              },
	{ "parm1",              OP_PARM1              },
	{ "parm2",              OP_PARM2              },
	{ "parm3",              OP_PARM3              },
	{ "parm4",              OP_PARM4              },
	{ "parm5",              OP_PARM5              },
	{ "parm6",              OP_PARM6              },
	{ "parm7",              OP_PARM7              },
	{ "parm8",              OP_PARM8              },
	{ "parm9",              OP_PARM9              },
	{ "parm10",             OP_PARM10             },
	{ "parm11",             OP_PARM11             },
	{ "global0",            OP_GLOBAL0            },
	{ "global1",            OP_GLOBAL1            },
	{ "global2",            OP_GLOBAL2            },
	{ "global3",            OP_GLOBAL3            },
	{ "global4",            OP_GLOBAL4            },
	{ "global5",            OP_GLOBAL5            },
	{ "global6",            OP_GLOBAL6            },
	{ "global7",            OP_GLOBAL7            },
	{ "fragmentShaders",    OP_FRAGMENTSHADERS    },
	{ "frameBufferObjects", OP_FRAMEBUFFEROBJECTS },
	{ "sound",              OP_SOUND              },
	{ "distance",           OP_DISTANCE           },
	{ "table",              OP_TABLE              },
	{ NULL,                 OP_BAD                }
};

static void GetOpType(char *token, expOperation_t *op)
{
	const opstring_t *opString;
	char             tableName[MAX_QPATH];
	int              hash;
	shaderTable_t    *tb;

	if ((token[0] >= '0' && token[0] <= '9') ||
	    //(token[0] == '-' && token[1] >= '0' && token[1] <= '9')   ||
	    //(token[0] == '+' && token[1] >= '0' && token[1] <= '9')   ||
	    (token[0] == '.' && token[1] >= '0' && token[1] <= '9'))
	{
		op->type = OP_NUM;
		return;
	}

	Q_strncpyz(tableName, token, sizeof(tableName));
	hash = generateHashValue(tableName, MAX_SHADERTABLE_HASH);

	for (tb = shaderTableHashTable[hash]; tb; tb = tb->next)
	{
		if (Q_stricmp(tb->name, tableName) == 0)
		{
			// match found
			op->type  = OP_TABLE;
			op->value = tb->index;
			return;
		}
	}

	for (opString = opStrings; opString->s; opString++)
	{
		if (!Q_stricmp(token, opString->s))
		{
			op->type = opString->type;
			return;
		}
	}

	op->type = OP_BAD;
}

static qboolean IsOperand(opcode_t oc)
{
	switch (oc)
	{
	case OP_NUM:
	case OP_TIME:
	case OP_PARM0:
	case OP_PARM1:
	case OP_PARM2:
	case OP_PARM3:
	case OP_PARM4:
	case OP_PARM5:
	case OP_PARM6:
	case OP_PARM7:
	case OP_PARM8:
	case OP_PARM9:
	case OP_PARM10:
	case OP_PARM11:
	case OP_GLOBAL0:
	case OP_GLOBAL1:
	case OP_GLOBAL2:
	case OP_GLOBAL3:
	case OP_GLOBAL4:
	case OP_GLOBAL5:
	case OP_GLOBAL6:
	case OP_GLOBAL7:
	case OP_FRAGMENTSHADERS:
	case OP_FRAMEBUFFEROBJECTS:
	case OP_SOUND:
	case OP_DISTANCE:
		return qtrue;
	default:
		return qfalse;
	}
}

static qboolean IsOperator(opcode_t oc)
{
	switch (oc)
	{
	case OP_LAND:
	case OP_LOR:
	case OP_GE:
	case OP_LE:
	case OP_LEQ:
	case OP_LNE:
	case OP_ADD:
	case OP_SUB:
	case OP_DIV:
	case OP_MOD:
	case OP_MUL:
	case OP_NEG:
	case OP_LT:
	case OP_GT:
	case OP_TABLE:
		return qtrue;
	default:
		return qfalse;
	}
}

static int GetOpPrecedence(opcode_t oc)
{
	switch (oc)
	{
	case OP_LOR:
		return 1;
	case OP_LAND:
		return 2;
	case OP_LEQ:
	case OP_LNE:
		return 3;
	case OP_GE:
	case OP_LE:
	case OP_LT:
	case OP_GT:
		return 4;
	case OP_ADD:
	case OP_SUB:
		return 5;
	case OP_DIV:
	case OP_MOD:
	case OP_MUL:
		return 6;
	case OP_NEG:
		return 7;
	case OP_TABLE:
		return 8;
	default:
		return 0;
	}
}

static char *ParseExpressionElement(char **data_p)
{
	int         c = 0, len;
	char        *data;
	const char  **punc;
	static char token[MAX_TOKEN_CHARS];
	// multiple character punctuation tokens
	const char *punctuation[] =
	{
		"&&", "||", "<=", ">=", "==", "!=", NULL
	};

	if (!data_p)
	{
		Ren_Fatal("ParseExpressionElement: NULL data_p");
	}

	data     = *data_p;
	len      = 0;
	token[0] = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return token;
	}

	// skip whitespace
	while (1)
	{
		// skip whitespace
		while ((c = *data) <= ' ')
		{
			if (!c)
			{
				*data_p = NULL;
				return token;
			}
			else if (c == '\n')
			{
				data++;
				*data_p = data;
				return token;
			}
			else
			{
				data++;
			}
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			data += 2;
			while (*data && *data != '\n')
			{
				data++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			data += 2;
			while (*data && (*data != '*' || data[1] != '/'))
			{
				data++;
			}
			if (*data)
			{
				data += 2;
			}
		}
		else
		{
			// a real token to parse
			break;
		}
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;

			if ((c == '\\') && (*data == '\"'))
			{
				// allow quoted strings to use \" to indicate the " character
				data++;
			}
			else if (c == '\"' || !c)
			{
				token[len] = 0;
				*data_p    = (char *)data;
				return token;
			}

			if (len < MAX_TOKEN_CHARS - 1)
			{
				token[len] = c;
				len++;
			}
		}
	}

	// check for a number
	if ((c >= '0' && c <= '9') ||
	    //(c == '-' && data[1] >= '0' && data[1] <= '9') ||
	    //(c == '+' && data[1] >= '0' && data[1] <= '9') ||
	    (c == '.' && data[1] >= '0' && data[1] <= '9'))
	{
		do
		{
			if (len < MAX_TOKEN_CHARS - 1)
			{
				token[len] = c;
				len++;
			}
			data++;

			c = *data;
		}
		while ((c >= '0' && c <= '9') || c == '.');

		// parse the exponent
		if (c == 'e' || c == 'E')
		{
			if (len < MAX_TOKEN_CHARS - 1)
			{
				token[len] = c;
				len++;
			}
			data++;
			c = *data;

			if (c == '-' || c == '+')
			{
				if (len < MAX_TOKEN_CHARS - 1)
				{
					token[len] = c;
					len++;
				}
				data++;
				c = *data;
			}

			do
			{
				if (len < MAX_TOKEN_CHARS - 1)
				{
					token[len] = c;
					len++;
				}
				data++;

				c = *data;
			}
			while (c >= '0' && c <= '9');
		}

		if (len == MAX_TOKEN_CHARS)
		{
			len = 0;
		}
		token[len] = 0;

		*data_p = (char *)data;
		return token;
	}

	// check for a regular word
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'))
	{
		do
		{
			if (len < MAX_TOKEN_CHARS - 1)
			{
				token[len] = c;
				len++;
			}
			data++;

			c = *data;
		}
		while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_') || (c >= '0' && c <= '9'));

		if (len == MAX_TOKEN_CHARS)
		{
			len = 0;
		}
		token[len] = 0;

		*data_p = (char *)data;
		return token;
	}

	// check for multi-character punctuation token
	for (punc = punctuation; *punc; punc++)
	{
		int l;
		int j;

		l = strlen(*punc);
		for (j = 0; j < l; j++)
		{
			if (data[j] != (*punc)[j])
			{
				break;
			}
		}
		if (j == l)
		{
			// a valid multi-character punctuation
			memcpy(token, *punc, l);
			token[l] = 0;
			data    += l;
			*data_p  = (char *)data;
			return token;
		}
	}

	// single character punctuation
	token[0] = *data;
	token[1] = 0;
	data++;
	*data_p = (char *)data;

	return token;
}

/*
===============
ParseExpression
===============
*/
static void ParseExpression(char **text, expression_t *exp)
{
	int            i;
	char           *token;
	expOperation_t op, op2;
	expOperation_t inFixOps[MAX_EXPRESSION_OPS];
	int            numInFixOps = 0;
	// convert stack
	expOperation_t tmpOps[MAX_EXPRESSION_OPS];
	int            numTmpOps = 0;

	exp->numOps = 0;

	// push left parenthesis on the stack
	op.type                 = OP_LPAREN;
	op.value                = 0;
	inFixOps[numInFixOps++] = op;

	while (1)
	{
		token = ParseExpressionElement(text);

		if (token[0] == 0 || token[0] == ',')
		{
			break;
		}

		if (numInFixOps == MAX_EXPRESSION_OPS)
		{
			Ren_Print("WARNING: too many arithmetic expression operations in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			return;
		}

		GetOpType(token, &op);

		switch (op.type)
		{
		case OP_BAD:
			Ren_Print("WARNING: unknown token '%s' for arithmetic expression in shader '%s'\n", token,
			          shader.name);
			break;
		case OP_LBRACKET:
			inFixOps[numInFixOps++] = op;

			// add extra (
			op2.type                = OP_LPAREN;
			op2.value               = 0;
			inFixOps[numInFixOps++] = op2;
			break;
		case OP_RBRACKET:
			// add extra )
			op2.type                = OP_RPAREN;
			op2.value               = 0;
			inFixOps[numInFixOps++] = op2;

			inFixOps[numInFixOps++] = op;
			break;
		case OP_NUM:
			op.value                = atof(token);
			inFixOps[numInFixOps++] = op;
			break;
		case OP_TABLE:
			// value already set by GetOpType
			inFixOps[numInFixOps++] = op;
			break;
		default:
			op.value                = 0;
			inFixOps[numInFixOps++] = op;
			break;
		}
	}

	// push right parenthesis on the stack
	op.type                 = OP_RPAREN;
	op.value                = 0;
	inFixOps[numInFixOps++] = op;

	for (i = 0; i < (numInFixOps - 1); i++)
	{
		op  = inFixOps[i];
		op2 = inFixOps[i + 1];

		// convert OP_SUBs that should be unary into OP_NEG
		if (op2.type == OP_SUB && op.type != OP_RPAREN && op.type != OP_TABLE && !IsOperand(op.type))
		{
			inFixOps[i + 1].type = OP_NEG;
		}
	}

#if 0
	Ren_Print("infix:\n");
	for (i = 0; i < numInFixOps; i++)
	{
		op = inFixOps[i];

		switch (op.type)
		{
		case OP_NUM:
			Ren_Print("%f ", op.value);
			break;
		case OP_TABLE:
			Ren_Print("%s ", tr.shaderTables[(int)op.value]->name);
			break;
		default:
			Ren_Print("%s ", opStrings[op.type].s);
			break;
		}
	}
	Ren_Print("\n");
#endif

	// http://cis.stvincent.edu/swd/stl/stacks/stacks.html
	// http://www.qiksearch.com/articles/cs/infix-postfix/
	// http://www.experts-exchange.com/Programming/Programming_Languages/C/Q_20394130.html

	// convert infix representation to postfix

	for (i = 0; i < numInFixOps; i++)
	{
		op = inFixOps[i];

		// if current operator in infix is digit
		if (IsOperand(op.type))
		{
			exp->ops[exp->numOps++] = op;
		}
		// if current operator in infix is left parenthesis
		else if (op.type == OP_LPAREN)
		{
			tmpOps[numTmpOps++] = op;
		}
		// if current operator in infix is operator
		else if (IsOperator(op.type))
		{
			while (qtrue)
			{
				if (!numTmpOps)
				{
					Ren_Print("WARNING: invalid infix expression in shader '%s'\n", shader.name);
					return;
				}
				else
				{
					// get top element
					op2 = tmpOps[numTmpOps - 1];

					if (IsOperator(op2.type))
					{
						if (GetOpPrecedence(op2.type) >= GetOpPrecedence(op.type))
						{
							exp->ops[exp->numOps++] = op2;
							numTmpOps--;
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
			}

			tmpOps[numTmpOps++] = op;
		}
		// if current operator in infix is right parenthesis
		else if (op.type == OP_RPAREN)
		{
			while (qtrue)
			{
				if (!numTmpOps)
				{
					Ren_Print("WARNING: invalid infix expression in shader '%s'\n", shader.name);
					return;
				}
				else
				{
					// get top element
					op2 = tmpOps[numTmpOps - 1];

					if (op2.type != OP_LPAREN)
					{
						exp->ops[exp->numOps++] = op2;
						numTmpOps--;
					}
					else
					{
						numTmpOps--;
						break;
					}
				}
			}
		}
	}

	// everything went ok
	exp->active = qtrue;

#if 0
	Ren_Print("postfix:\n");
	for (i = 0; i < exp->numOps; i++)
	{
		op = exp->ops[i];

		switch (op.type)
		{
		case OP_NUM:
			Ren_Print("%f ", op.value);
			break;
		case OP_TABLE:
			Ren_Print("%s ", tr.shaderTables[(int)op.value]->name);
			break;
		default:
			Ren_Print("%s ", opStrings[op.type].s);
			break;
		}
	}
	Ren_Print("\n");
#endif
}

/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc(const char *funcname)
{
	if (!Q_stricmp(funcname, "GT0"))
	{
		return GLS_ATEST_GT_0;
	}
	else if (!Q_stricmp(funcname, "LT128"))
	{
		return GLS_ATEST_LT_128;
	}
	else if (!Q_stricmp(funcname, "GE128"))
	{
		return GLS_ATEST_GE_128;
	}

	Ren_Warning("WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name);
	return 0;
}

/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode(const char *name)
{
	if (!Q_stricmp(name, "GL_ONE"))
	{
		return GLS_SRCBLEND_ONE;
	}
	else if (!Q_stricmp(name, "GL_ZERO"))
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if (!Q_stricmp(name, "GL_DST_COLOR"))
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_COLOR"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_DST_ALPHA"))
	{
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA_SATURATE"))
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	Ren_Warning("WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode(const char *name)
{
	if (!Q_stricmp(name, "GL_ONE"))
	{
		return GLS_DSTBLEND_ONE;
	}
	else if (!Q_stricmp(name, "GL_ZERO"))
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_DST_ALPHA"))
	{
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_SRC_COLOR"))
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_COLOR"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	Ren_Warning("WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc(const char *funcname)
{
	if (!Q_stricmp(funcname, "sin"))
	{
		return GF_SIN;
	}
	else if (!Q_stricmp(funcname, "square"))
	{
		return GF_SQUARE;
	}
	else if (!Q_stricmp(funcname, "triangle"))
	{
		return GF_TRIANGLE;
	}
	else if (!Q_stricmp(funcname, "sawtooth"))
	{
		return GF_SAWTOOTH;
	}
	else if (!Q_stricmp(funcname, "inversesawtooth"))
	{
		return GF_INVERSE_SAWTOOTH;
	}
	else if (!Q_stricmp(funcname, "noise"))
	{
		return GF_NOISE;
	}

	Ren_Warning("WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name);
	return GF_SIN;
}

/*
===============
NameToStencilOp
===============
*/
static int NameToStencilOp(char *name)
{
	if (!Q_stricmp(name, "keep"))
	{
		return STO_KEEP;
	}
	else if (!Q_stricmp(name, "zero"))
	{
		return STO_ZERO;
	}
	else if (!Q_stricmp(name, "replace"))
	{
		return STO_REPLACE;
	}
	else if (!Q_stricmp(name, "invert"))
	{
		return STO_INVERT;
	}
	else if (!Q_stricmp(name, "incr"))
	{
		return STO_INCR;
	}
	else if (!Q_stricmp(name, "decr"))
	{
		return STO_DECR;
	}
	else
	{
		Ren_Warning("WARNING: invalid stencil op name '%s' in shader '%s'\n", name, shader.name);
		return STO_KEEP;
	}
}

/*
===============
ParseStencil
===============
*/
static void ParseStencil(char **text, stencil_t *stencil)
{
	char *token;

	stencil->flags = 0;
	stencil->mask  = stencil->writeMask = 0xff;
	stencil->ref   = 1;

	// [mask <mask>]
	token = COM_ParseExt(text, qfalse);

	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing stencil ref value in shader '%s'\n", shader.name);
		return;
	}

	if (!Q_stricmp(token, "mask"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stencil mask value in shader '%s'\n", shader.name);
			return;
		}
		stencil->mask = atoi(token);

		token = COM_ParseExt(text, qfalse);
	}

	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing stencil ref value in shader '%s'\n", shader.name);
		return;
	}

	if (!Q_stricmp(token, "writeMask"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stencil writeMask value in shader '%s'\n", shader.name);
			return;
		}
		stencil->writeMask = atoi(token);

		token = COM_ParseExt(text, qfalse);
	}

	// <ref>
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing stencil ref value in shader '%s'\n", shader.name);
		return;
	}

	stencil->ref = atoi(token);

	// <op>
	token = COM_ParseExt(text, qfalse);

	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing stencil test op in shader '%s'\n", shader.name);
		return;
	}
	else if (!Q_stricmp(token, "always"))
	{
		stencil->flags |= STF_ALWAYS;
	}
	else if (!Q_stricmp(token, "never"))
	{
		stencil->flags |= STF_NEVER;
	}
	else if (!Q_stricmp(token, "less"))
	{
		stencil->flags |= STF_LESS;
	}
	else if (!Q_stricmp(token, "lequal"))
	{
		stencil->flags |= STF_LEQUAL;
	}
	else if (!Q_stricmp(token, "greater"))
	{
		stencil->flags |= STF_GREATER;
	}
	else if (!Q_stricmp(token, "gequal"))
	{
		stencil->flags |= STF_GEQUAL;
	}
	else if (!Q_stricmp(token, "equal"))
	{
		stencil->flags |= STF_EQUAL;
	}
	else if (!Q_stricmp(token, "nequal"))
	{
		stencil->flags |= STF_NEQUAL;
	}
	else
	{
		Ren_Warning("WARNING: missing stencil test op in shader '%s'\n", shader.name);
		return;
	}

	// <sfail>
	token = COM_ParseExt(text, qfalse);

	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing stencil sfail op in shader '%s'\n", shader.name);
		return;
	}
	stencil->flags |= NameToStencilOp(token) << STS_SFAIL;

	// <zfail>
	token = COM_ParseExt(text, qfalse);

	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing stencil zfail op in shader '%s'\n", shader.name);
		return;
	}
	stencil->flags |= NameToStencilOp(token) << STS_ZFAIL;

	// <zpass>
	token = COM_ParseExt(text, qfalse);

	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing stencil zpass op in shader '%s'\n", shader.name);
		return;
	}
	stencil->flags |= NameToStencilOp(token) << STS_ZPASS;
}

/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm(char **text, waveForm_t *wave)
{
	char *token;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->func = NameToGenFunc(token);

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->base = atof(token);

	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->amplitude = atof(token);

	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->phase = atof(token);

	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->frequency = atof(token);
}

/*
===================
ParseTexMod
===================
*/
static qboolean ParseTexMod(char **text, shaderStage_t *stage)
{
	const char   *token;
	texModInfo_t *tmi;

	if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
	{
		Ren_Drop("ERROR: too many tcMod stages in shader '%s'\n", shader.name);
		return qfalse;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = COM_ParseExt2(text, qfalse);

	//Ren_Print("using tcMod '%s' in shader '%s'\n", token, shader.name);

	// turb
	if (!Q_stricmp(token, "turb"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod turb parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.base = atof(token);
		token          = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.amplitude = atof(token);
		token               = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.phase = atof(token);
		token           = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_TURBULENT;
	}
	// scale
	else if (!Q_stricmp(token, "scale"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing scale parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->scale[0] = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing scale parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->scale[1] = atof(token);
		tmi->type     = TMOD_SCALE;
	}
	// scroll
	else if (!Q_stricmp(token, "scroll"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing scale scroll parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->scroll[0] = atof(token);
		token          = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing scale scroll parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->scroll[1] = atof(token);
		tmi->type      = TMOD_SCROLL;
	}
	// stretch
	else if (!Q_stricmp(token, "stretch"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.func = NameToGenFunc(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.base = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.amplitude = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.phase = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_STRETCH;
	}
	// transform
	else if (!Q_stricmp(token, "transform"))
	{
		MatrixIdentity(tmi->matrix);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->matrix[0] = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->matrix[1] = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->matrix[4] = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->matrix[5] = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->matrix[12] = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->matrix[13] = atof(token);

		tmi->type = TMOD_TRANSFORM;
	}
	// rotate
	else if (!Q_stricmp(token, "rotate"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name);
			return qfalse;
		}
		tmi->rotateSpeed = atof(token);
		tmi->type        = TMOD_ROTATE;
	}
	// entityTranslate
	else if (!Q_stricmp(token, "entityTranslate"))
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		Ren_Warning("WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name);
		return qfalse;
	}

	// NOTE: some shaders using tcMod are messed up by artists so we need this bugfix
	while (1)
	{
		token = COM_ParseExt2(text, qfalse);

		if (token[0] == 0)
		{
			break;
		}

		Ren_Warning("WARNING: obsolete tcMod parameter '%s' in shader '%s'\n", token, shader.name);
	}

	return qtrue;
}

static qboolean ParseMap(shaderStage_t *stage, char **text, char *buffer, int bufferSize)
{
	int  len;
	char *token;

	// examples
	// map textures/caves/tembrick1crum_local.tga
	// addnormals (textures/caves/tembrick1crum_local.tga, heightmap (textures/caves/tembrick1crum_bmp.tga, 3 ))
	// heightmap( textures/hell/hellbones_d07bbump.tga, 8)

	while (1)
	{
		token = COM_ParseExt2(text, qfalse);

		if (!token[0])
		{
			// end of line
			break;
		}

		Q_strcat(buffer, bufferSize, token);
		Q_strcat(buffer, bufferSize, " ");
	}

	if (!buffer[0])
	{
		Ren_Warning("WARNING: 'map' missing parameter in shader '%s'\n", shader.name);
		return qfalse;
	}

	len             = strlen(buffer);
	buffer[len - 1] = 0;        // replace last ' ' with tailing zero

	return qtrue;
}

static qboolean LoadMap(shaderStage_t *stage, char *buffer)
{
	char         *token;
	int          imageBits = 0;
	filterType_t filterType;
	wrapType_t   wrapType;
	char         *buffer_p = &buffer[0];

	if (!buffer || !buffer[0])
	{
		Ren_Warning("WARNING: NULL parameter for LoadMap in shader '%s'\n", shader.name);
		return qfalse;
	}

	//Ren_Print("LoadMap: buffer '%s'\n", buffer);

	token = COM_ParseExt2(&buffer_p, qfalse);

	if (!Q_stricmp(token, "$whiteimage") || !Q_stricmp(token, "$white") || !Q_stricmp(token, "_white") ||
	    !Q_stricmp(token, "*white"))
	{
		stage->bundle[0].image[0] = tr.whiteImage;
		return qtrue;
	}
	else if (!Q_stricmp(token, "$blackimage") || !Q_stricmp(token, "$black") || !Q_stricmp(token, "_black") ||
	         !Q_stricmp(token, "*black"))
	{
		stage->bundle[0].image[0] = tr.blackImage;
		return qtrue;
	}
	else if (!Q_stricmp(token, "$flatimage") || !Q_stricmp(token, "$flat") || !Q_stricmp(token, "_flat") ||
	         !Q_stricmp(token, "*flat"))
	{
		stage->bundle[0].image[0] = tr.flatImage;
		return qtrue;
	}
	else if (!Q_stricmp(token, "$lightmap"))
	{
		stage->type = ST_LIGHTMAP;
		return qtrue;
	}

	// determine image options
	if (stage->overrideNoPicMip || shader.noPicMip || stage->highQuality || stage->forceHighQuality)
	{
		imageBits |= IF_NOPICMIP;
	}

	if (stage->type == ST_NORMALMAP || stage->type == ST_HEATHAZEMAP || stage->type == ST_LIQUIDMAP)
	{
		imageBits |= IF_NORMALMAP;
	}

	if (stage->type == ST_NORMALMAP && shader.parallax)
	{
		imageBits |= IF_DISPLACEMAP;
	}

	if (stage->uncompressed || stage->highQuality || stage->forceHighQuality || shader.uncompressed)
	{
		imageBits |= IF_NOCOMPRESSION;
	}

	if (stage->forceHighQuality)
	{
		imageBits |= IF_NOCOMPRESSION;
	}

	if (stage->stateBits & (GLS_ATEST_BITS))
	{
		imageBits |= IF_ALPHATEST;
	}

	if (stage->overrideFilterType)
	{
		filterType = stage->filterType;
	}
	else
	{
		filterType = shader.filterType;
	}

	if (stage->overrideWrapType)
	{
		wrapType = stage->wrapType;
	}
	else
	{
		wrapType = shader.wrapType;
	}

	// try to load the image
	stage->bundle[0].image[0] = R_FindImageFile(buffer, imageBits, filterType, wrapType, shader.name);

	if (!stage->bundle[0].image[0])
	{
		Ren_Warning("WARNING: R_FindImageFile could not find image '%s' in shader '%s'\n", buffer, shader.name);
		return qfalse;
	}

	return qtrue;
}

/*
===================
ParseStage
===================
*/
static qboolean ParseStage(shaderStage_t *stage, char **text)
{
	char *token;
	int  colorMaskBits             = 0;
	int  depthMaskBits             = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits =
	    0, polyModeBits            = 0;
	qboolean     depthMaskExplicit = qfalse;
	int          imageBits         = 0;
	filterType_t filterType;
	char         buffer[1024] = "";
	qboolean     loadMap      = qfalse;

	while (1)
	{
		token = COM_ParseExt2(text, qtrue);
		if (!token[0])
		{
			Ren_Warning("WARNING: no matching '}' found\n");
			return qfalse;
		}

		if (token[0] == '}')
		{
			break;
		}
		// if(<condition>)
		else if (!Q_stricmp(token, "if"))
		{
			ParseExpression(text, &stage->ifExp);
		}
		// map <name>
		else if (!Q_stricmp(token, "map"))
		{
			if (!ParseMap(stage, text, buffer, sizeof(buffer)))
			{
				Ren_Warning("WARNING: ParseMap could not create '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
			else
			{
				loadMap = qtrue;
			}
		}
		// lightmap <name>
		else if (!Q_stricmp(token, "lightmap"))
		{
			if (!ParseMap(stage, text, buffer, sizeof(buffer)))
			{
				Ren_Warning("WARNING: ParseMap could not create '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
			else
			{
				loadMap = qtrue;
			}

			//stage->type = ST_LIGHTMAP;
		}
		// remoteRenderMap <int> <int>
		else if (!Q_stricmp(token, "remoteRenderMap"))
		{
			Ren_Warning("WARNING: remoteRenderMap keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// mirrorRenderMap <int> <int>
		else if (!Q_stricmp(token, "mirrorRenderMap"))
		{
			Ren_Warning("WARNING: mirrorRenderMap keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// clampmap <name>
		else if (!Q_stricmp(token, "clampmap"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			imageBits = 0;
			if (stage->overrideNoPicMip || shader.noPicMip)
			{
				imageBits |= IF_NOPICMIP;
			}

			if (stage->overrideFilterType)
			{
				filterType = stage->filterType;
			}
			else
			{
				filterType = shader.filterType;
			}

			stage->bundle[0].image[0] = R_FindImageFile(token, imageBits, filterType, WT_CLAMP, shader.name);
			if (!stage->bundle[0].image[0])
			{
				Ren_Warning("WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}
		// animMap <frequency> <image1> .... <imageN>
		else if (!Q_stricmp(token, "animMap"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof(token);

			imageBits = 0;
			if (stage->overrideNoPicMip || shader.noPicMip)
			{
				imageBits |= IF_NOPICMIP;
			}

			if (stage->overrideFilterType)
			{
				filterType = stage->filterType;
			}
			else
			{
				filterType = shader.filterType;
			}

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while (1)
			{
				int num;

				token = COM_ParseExt2(text, qfalse);
				if (!token[0])
				{
					break;
				}
				num = stage->bundle[0].numImages;
				if (num < MAX_IMAGE_ANIMATIONS)
				{
					stage->bundle[0].image[num] = R_FindImageFile(token, imageBits, filterType, WT_REPEAT, shader.name);
					if (!stage->bundle[0].image[num])
					{
						Ren_Warning("WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token,
						            shader.name);
						return qfalse;
					}
					stage->bundle[0].numImages++;
				}
			}
		}
		else if (!Q_stricmp(token, "videoMap"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri.CIN_PlayCinematic(token, 0, 0, 512, 512, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[0].videoMapHandle != -1)
			{
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0]   = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		// soundmap [waveform]
		else if (!Q_stricmp(token, "soundMap"))
		{
			Ren_Warning("WARNING: soundMap keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// cubeMap <map>
		else if (!Q_stricmp(token, "cubeMap") || !Q_stricmp(token, "cameraCubeMap"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'cubeMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			imageBits = 0;
			if (stage->overrideNoPicMip || shader.noPicMip)
			{
				imageBits |= IF_NOPICMIP;
			}

			if (stage->overrideFilterType)
			{
				filterType = stage->filterType;
			}
			else
			{
				filterType = shader.filterType;
			}

			stage->bundle[0].image[0] = R_FindCubeImage(token, imageBits, filterType, WT_EDGE_CLAMP, shader.name);
			if (!stage->bundle[0].image[0])
			{
				Ren_Warning("WARNING: R_FindCubeImage could not find '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}
		// alphafunc <func>
		else if (!Q_stricmp(token, "alphaFunc"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			atestBits = NameToAFunc(token);
		}
		// alphaTest <exp>
		else if (!Q_stricmp(token, "alphaTest"))
		{
			atestBits = GLS_ATEST_GE_128;
			ParseExpression(text, &stage->alphaTestExp);
		}
		// depthFunc <func>
		else if (!Q_stricmp(token, "depthfunc"))
		{
			token = COM_ParseExt2(text, qfalse);

			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "lequal"))
			{
				depthFuncBits = 0;
			}
			else if (!Q_stricmp(token, "equal"))
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else
			{
				Ren_Warning("WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		// stencil <side> [mask <mask>] [writeMask <mask>] <ref> <op> <sfail> <zfail> <zpass>
		else if (!Q_stricmp(token, "stencil"))
		{
			token = COM_ParseExt(text, qfalse);

			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'stencil' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "front"))
			{
				ParseStencil(text, &stage->frontStencil);
			}
			else if (!Q_stricmp(token, "back"))
			{
				ParseStencil(text, &stage->backStencil);
			}
			else if (!Q_stricmp(token, "both"))
			{
				ParseStencil(text, &stage->frontStencil);
				Com_Memcpy(&stage->backStencil, &stage->frontStencil, sizeof(stencil_t));
			}
			else
			{
				Ren_Warning("WARNING: unknown stencil side '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		// ignoreAlphaTest
		else if (!Q_stricmp(token, "ignoreAlphaTest"))
		{
			depthFuncBits = 0;
		}
		// nearest
		else if (!Q_stricmp(token, "nearest"))
		{
			stage->overrideFilterType = qtrue;
			stage->filterType         = FT_NEAREST;
		}
		// linear
		else if (!Q_stricmp(token, "linear"))
		{
			stage->overrideFilterType = qtrue;
			stage->filterType         = FT_LINEAR;

			stage->overrideNoPicMip = qtrue;
		}
		// noPicMip
		else if (!Q_stricmp(token, "noPicMip"))
		{
			stage->overrideNoPicMip = qtrue;
		}
		// clamp
		else if (!Q_stricmp(token, "clamp"))
		{
			stage->overrideWrapType = qtrue;
			stage->wrapType         = WT_CLAMP;
		}
		// edgeClamp
		else if (!Q_stricmp(token, "edgeClamp"))
		{
			stage->overrideWrapType = qtrue;
			stage->wrapType         = WT_EDGE_CLAMP;
		}
		// zeroClamp
		else if (!Q_stricmp(token, "zeroClamp"))
		{
			stage->overrideWrapType = qtrue;
			stage->wrapType         = WT_ZERO_CLAMP;
		}
		// alphaZeroClamp
		else if (!Q_stricmp(token, "alphaZeroClamp"))
		{
			stage->overrideWrapType = qtrue;
			stage->wrapType         = WT_ALPHA_ZERO_CLAMP;
		}
		// noClamp
		else if (!Q_stricmp(token, "noClamp"))
		{
			stage->overrideWrapType = qtrue;
			stage->wrapType         = WT_REPEAT;
		}
		// uncompressed
		else if (!Q_stricmp(token, "uncompressed"))
		{
			stage->uncompressed = qtrue;
		}
		// highQuality
		else if (!Q_stricmp(token, "highQuality"))
		{
			stage->highQuality      = qtrue;
			stage->overrideNoPicMip = qtrue;
		}
		// forceHighQuality
		else if (!Q_stricmp(token, "forceHighQuality"))
		{
			stage->forceHighQuality = qtrue;
			stage->overrideNoPicMip = qtrue;
		}
		// detail
		else if (!Q_stricmp(token, "detail"))
		{
			stage->isDetail = qtrue;
		}
		// ET fog
		else if (!Q_stricmp(token, "fog"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parm for fog in shader '%s'\n", shader.name);
				continue;
			}
			if (!Q_stricmp(token, "on"))
			{
				stage->noFog = qfalse;
			}
			else
			{
				stage->noFog = qtrue;
			}
		}
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		else if (!Q_stricmp(token, "blendfunc"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
				continue;
			}
			// check for "simple" blends first
			if (!Q_stricmp(token, "add"))
			{
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			else if (!Q_stricmp(token, "filter"))
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if (!Q_stricmp(token, "blend"))
			{
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
			else
			{
				// complex double blends
				blendSrcBits = NameToSrcBlendMode(token);

				token = COM_ParseExt2(text, qfalse);
				if (token[0] == 0)
				{
					Ren_Warning("WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
					continue;
				}
				blendDstBits = NameToDstBlendMode(token);
			}

			// clear depth mask for blended surfaces
			if (!depthMaskExplicit)
			{
				depthMaskBits = 0;
			}
		}
		// blend <srcFactor> , <dstFactor>
		// or blend <add | filter | blend>
		// or blend <diffusemap | bumpmap | specularmap>
		else if (!Q_stricmp(token, "blend"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parm for blend in shader '%s'\n", shader.name);
				continue;
			}

			// check for "simple" blends first
			if (!Q_stricmp(token, "add"))
			{
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			else if (!Q_stricmp(token, "filter"))
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if (!Q_stricmp(token, "modulate"))
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if (!Q_stricmp(token, "blend"))
			{
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
			else if (!Q_stricmp(token, "none"))
			{
				blendSrcBits = GLS_SRCBLEND_ZERO;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			// check for other semantic meanings
			else if (!Q_stricmp(token, "diffuseMap"))
			{
				stage->type = ST_DIFFUSEMAP;
			}
			else if (!Q_stricmp(token, "bumpMap"))
			{
				stage->type = ST_NORMALMAP;
			}
			else if (!Q_stricmp(token, "specularMap"))
			{
				stage->type = ST_SPECULARMAP;
			}
			else
			{
				// complex double blends
				blendSrcBits = NameToSrcBlendMode(token);

				token = COM_ParseExt2(text, qfalse);
				if (token[0] != ',')
				{
					Ren_Warning("WARNING: expecting ',', found '%s' instead for blend in shader '%s'\n", token,
					            shader.name);
					continue;
				}

				token = COM_ParseExt2(text, qfalse);
				if (token[0] == 0)
				{
					Ren_Warning("WARNING: missing parm for blend in shader '%s'\n", shader.name);
					continue;
				}
				blendDstBits = NameToDstBlendMode(token);
			}

			// clear depth mask for blended surfaces
			if (!depthMaskExplicit && stage->type == ST_COLORMAP)
			{
				depthMaskBits = 0;
			}
		}
		// stage <type>
		else if (!Q_stricmp(token, "stage"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parameters for stage in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "colorMap"))
			{
				stage->type = ST_COLORMAP;
			}
			else if (!Q_stricmp(token, "diffuseMap"))
			{
				stage->type = ST_DIFFUSEMAP;
			}
			else if (!Q_stricmp(token, "normalMap") || !Q_stricmp(token, "bumpMap"))
			{
				stage->type = ST_NORMALMAP;
			}
			else if (!Q_stricmp(token, "specularMap"))
			{
				stage->type = ST_SPECULARMAP;
			}
			else if (!Q_stricmp(token, "reflectionMap"))
			{
				stage->type = ST_REFLECTIONMAP;
			}
			else if (!Q_stricmp(token, "refractionMap"))
			{
				stage->type = ST_REFRACTIONMAP;
			}
			else if (!Q_stricmp(token, "dispersionMap"))
			{
				stage->type = ST_DISPERSIONMAP;
			}
			else if (!Q_stricmp(token, "skyboxMap"))
			{
				stage->type = ST_SKYBOXMAP;
			}
			else if (!Q_stricmp(token, "screenMap"))
			{
				stage->type = ST_SCREENMAP;
			}
			else if (!Q_stricmp(token, "portalMap"))
			{
				stage->type = ST_PORTALMAP;
			}
			else if (!Q_stricmp(token, "heathazeMap"))
			{
				stage->type = ST_HEATHAZEMAP;
			}
			else if (!Q_stricmp(token, "liquidMap"))
			{
				stage->type = ST_LIQUIDMAP;
			}
			else if (!Q_stricmp(token, "attenuationMapXY"))
			{
				stage->type = ST_ATTENUATIONMAP_XY;
			}
			else if (!Q_stricmp(token, "attenuationMapZ"))
			{
				stage->type = ST_ATTENUATIONMAP_Z;
			}
			else
			{
				Ren_Warning("WARNING: unknown stage parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		// rgbGen
		else if (!Q_stricmp(token, "rgbGen"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->rgbWave);
				stage->rgbGen = CGEN_WAVEFORM;
			}
			else if (!Q_stricmp(token, "const"))
			{
				vec3_t color;

				ParseVector(text, 3, color);
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			}
			else if (!Q_stricmp(token, "identity"))
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if (!Q_stricmp(token, "identityLighting"))
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if (!Q_stricmp(token, "entity"))
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusEntity"))
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if (!Q_stricmp(token, "vertex"))
			{
				stage->rgbGen = CGEN_VERTEX;
				if (stage->alphaGen == 0)
				{
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if (!Q_stricmp(token, "exactVertex"))
			{
				stage->rgbGen = CGEN_VERTEX;
			}
			else if (!Q_stricmp(token, "lightingDiffuse"))
			{
				//Ren_Warning( "WARNING: obsolete rgbGen lightingDiffuse keyword not supported in shader '%s'\n", shader.name);
				stage->type   = ST_DIFFUSEMAP;
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if (!Q_stricmp(token, "oneMinusVertex"))
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				Ren_Warning("WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		// rgb <arithmetic expression>
		else if (!Q_stricmp(token, "rgb"))
		{
			stage->rgbGen = CGEN_CUSTOM_RGB;
			ParseExpression(text, &stage->rgbExp);
		}
		// red <arithmetic expression>
		else if (!Q_stricmp(token, "red"))
		{
			stage->rgbGen = CGEN_CUSTOM_RGBs;
			ParseExpression(text, &stage->redExp);
		}
		// green <arithmetic expression>
		else if (!Q_stricmp(token, "green"))
		{
			stage->rgbGen = CGEN_CUSTOM_RGBs;
			ParseExpression(text, &stage->greenExp);
		}
		// blue <arithmetic expression>
		else if (!Q_stricmp(token, "blue"))
		{
			stage->rgbGen = CGEN_CUSTOM_RGBs;
			ParseExpression(text, &stage->blueExp);
		}
		// colored
		else if (!Q_stricmp(token, "colored"))
		{
			stage->rgbGen   = CGEN_ENTITY;
			stage->alphaGen = AGEN_ENTITY;
		}
		// vertexColor
		else if (!Q_stricmp(token, "vertexColor"))
		{
			stage->rgbGen = CGEN_VERTEX;
			if (stage->alphaGen == 0)
			{
				stage->alphaGen = AGEN_VERTEX;
			}
		}
		// inverseVertexColor
		else if (!Q_stricmp(token, "inverseVertexColor"))
		{
			stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			if (stage->alphaGen == 0)
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
		}
		// alphaGen
		else if (!Q_stricmp(token, "alphaGen"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->alphaWave);
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if (!Q_stricmp(token, "const"))
			{
				token                   = COM_ParseExt2(text, qfalse);
				stage->constantColor[3] = 255 * atof(token);
				stage->alphaGen         = AGEN_CONST;
			}
			else if (!Q_stricmp(token, "identity"))
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if (!Q_stricmp(token, "entity"))
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusEntity"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if (!Q_stricmp(token, "normalzfade"))
			{
				stage->alphaGen = AGEN_NORMALZFADE;
				token           = COM_ParseExt(text, qfalse);
				if (token[0])
				{
					stage->constantColor[3] = 255 * atof(token);
				}
				else
				{
					stage->constantColor[3] = 255;
				}

				token = COM_ParseExt(text, qfalse);
				if (token[0])
				{
					stage->zFadeBounds[0] = atof(token);    // lower range
					token                 = COM_ParseExt(text, qfalse);
					stage->zFadeBounds[1] = atof(token);    // upper range
				}
				else
				{
					stage->zFadeBounds[0] = -1.0;   // lower range
					stage->zFadeBounds[1] = 1.0;    // upper range
				}

			}
			else if (!Q_stricmp(token, "vertex"))
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if (!Q_stricmp(token, "lightingSpecular"))
			{
				Ren_Warning("WARNING: alphaGen lightingSpecular keyword not supported in shader '%s'\n", shader.name);
			}
			else if (!Q_stricmp(token, "oneMinusVertex"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if (!Q_stricmp(token, "portal"))
			{
				Ren_Warning("WARNING: alphaGen portal keyword not supported in shader '%s'\n", shader.name);
				//stage->type = ST_PORTALMAP;
				stage->alphaGen         = AGEN_CONST;
				stage->constantColor[3] = 0;
				SkipRestOfLine(text);
			}
			else
			{
				Ren_Warning("WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		// alpha <arithmetic expression>
		else if (!Q_stricmp(token, "alpha"))
		{
			stage->alphaGen = AGEN_CUSTOM;
			ParseExpression(text, &stage->alphaExp);
		}
		// color <exp>, <exp>, <exp>, <exp>
		else if (!Q_stricmp(token, "color"))
		{
			stage->rgbGen   = CGEN_CUSTOM_RGBs;
			stage->alphaGen = AGEN_CUSTOM;
			ParseExpression(text, &stage->redExp);
			ParseExpression(text, &stage->greenExp);
			ParseExpression(text, &stage->blueExp);
			ParseExpression(text, &stage->alphaExp);
		}
		// privatePolygonOffset <float>
		else if (!Q_stricmp(token, "privatePolygonOffset"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'privatePolygonOffset' keyword in shader '%s'\n",
				            shader.name);
				return qfalse;
			}

			stage->privatePolygonOffset      = qtrue;
			stage->privatePolygonOffsetValue = atof(token);
		}
		// tcGen <function>
		else if (!Q_stricmp(token, "texGen") || !Q_stricmp(token, "tcGen"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing texGen parm in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "environment"))
			{
				//Ren_Warning( "WARNING: texGen environment keyword not supported in shader '%s'\n", shader.name);
				stage->tcGen_Environment = qtrue;
			}
			else if (!Q_stricmp(token, "lightmap"))
			{
				stage->tcGen_Lightmap = qtrue;
			}
			else if (!Q_stricmp(token, "texture") || !Q_stricmp(token, "base"))
			{
				Ren_Warning("WARNING: texGen texture/base keyword not supported in shader '%s'\n", shader.name);
			}
			else if (!Q_stricmp(token, "vector"))
			{
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[0]);
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[1]);

				stage->bundle[0].isTcGen = qtrue;
			}
			else if (!Q_stricmp(token, "reflect"))
			{
				//Ren_Warning( "WARNING: use stage reflectionmap instead of texGen reflect keyword shader '%s'\n",
				//		  shader.name);
				stage->type = ST_REFLECTIONMAP;
			}
			else if (!Q_stricmp(token, "skybox"))
			{
				//Ren_Warning( "WARNING: use stage skyboxmap instead of texGen skybox keyword shader '%s'\n",
				//		  shader.name);
				stage->type = ST_SKYBOXMAP;
			}
			else
			{
				Ren_Warning("WARNING: unknown tcGen keyword '%s' not supported in shader '%s'\n", token, shader.name);
				SkipRestOfLine(text);
			}
		}
		// tcMod <type> <...>
		else if (!Q_stricmp(token, "tcMod"))
		{
			if (!ParseTexMod(text, stage))
			{
				return qfalse;
			}
		}
		// scroll
		else if (!Q_stricmp(token, "scroll") || !Q_stricmp(token, "translate"))
		{
			texModInfo_t *tmi;

			if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
			{
				Ren_Drop("ERROR: too many tcMod stages in shader '%s'\n", shader.name);
				return qfalse;
			}

			tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
			stage->bundle[0].numTexMods++;

			ParseExpression(text, &tmi->sExp);
			ParseExpression(text, &tmi->tExp);

			tmi->type = TMOD_SCROLL2;
		}
		// scale
		else if (!Q_stricmp(token, "scale"))
		{
			texModInfo_t *tmi;

			if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
			{
				Ren_Drop("ERROR: too many tcMod stages in shader '%s'\n", shader.name);
				return qfalse;
			}

			tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
			stage->bundle[0].numTexMods++;

			ParseExpression(text, &tmi->sExp);
			ParseExpression(text, &tmi->tExp);

			tmi->type = TMOD_SCALE2;
		}
		// centerScale
		else if (!Q_stricmp(token, "centerScale"))
		{
			texModInfo_t *tmi;

			if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
			{
				Ren_Drop("ERROR: too many tcMod stages in shader '%s'\n", shader.name);
				return qfalse;
			}

			tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
			stage->bundle[0].numTexMods++;

			ParseExpression(text, &tmi->sExp);
			ParseExpression(text, &tmi->tExp);

			tmi->type = TMOD_CENTERSCALE;
		}
		// shear
		else if (!Q_stricmp(token, "shear"))
		{
			texModInfo_t *tmi;

			if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
			{
				Ren_Drop("ERROR: too many tcMod stages in shader '%s'\n", shader.name);
				return qfalse;
			}

			tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
			stage->bundle[0].numTexMods++;

			ParseExpression(text, &tmi->sExp);
			ParseExpression(text, &tmi->tExp);

			tmi->type = TMOD_SHEAR;
		}
		// rotate
		else if (!Q_stricmp(token, "rotate"))
		{
			texModInfo_t *tmi;

			if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
			{
				Ren_Drop("ERROR: too many tcMod stages in shader '%s'\n", shader.name);
				return qfalse;
			}

			tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
			stage->bundle[0].numTexMods++;

			ParseExpression(text, &tmi->rExp);

			tmi->type = TMOD_ROTATE2;
		}
		// depthwrite
		else if (!Q_stricmp(token, "depthwrite"))
		{
			depthMaskBits     = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;
			continue;
		}
		// maskRed
		else if (!Q_stricmp(token, "maskRed"))
		{
			colorMaskBits |= GLS_REDMASK_FALSE;
		}
		// maskGreen
		else if (!Q_stricmp(token, "maskGreen"))
		{
			colorMaskBits |= GLS_GREENMASK_FALSE;
		}
		// maskBlue
		else if (!Q_stricmp(token, "maskBlue"))
		{
			colorMaskBits |= GLS_BLUEMASK_FALSE;
		}
		// maskAlpha
		else if (!Q_stricmp(token, "maskAlpha"))
		{
			colorMaskBits |= GLS_ALPHAMASK_FALSE;
		}
		// maskColor
		else if (!Q_stricmp(token, "maskColor"))
		{
			colorMaskBits |= GLS_REDMASK_FALSE | GLS_GREENMASK_FALSE | GLS_BLUEMASK_FALSE;
		}
		// maskColorAlpha
		else if (!Q_stricmp(token, "maskColorAlpha"))
		{
			colorMaskBits |= GLS_REDMASK_FALSE | GLS_GREENMASK_FALSE | GLS_BLUEMASK_FALSE | GLS_ALPHAMASK_FALSE;
		}
		// maskDepth
		else if (!Q_stricmp(token, "maskDepth"))
		{
			depthMaskBits    &= ~GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qfalse;
		}
		// wireFrame
		else if (!Q_stricmp(token, "wireFrame"))
		{
			polyModeBits |= GLS_POLYMODE_LINE;
		}
		// refractionIndex <arithmetic expression>
		else if (!Q_stricmp(token, "refractionIndex"))
		{
			ParseExpression(text, &stage->refractionIndexExp);
		}
		// fresnelPower <arithmetic expression>
		else if (!Q_stricmp(token, "fresnelPower"))
		{
			ParseExpression(text, &stage->fresnelPowerExp);
		}
		// fresnelScale <arithmetic expression>
		else if (!Q_stricmp(token, "fresnelScale"))
		{
			ParseExpression(text, &stage->fresnelScaleExp);
		}
		// fresnelBias <arithmetic expression>
		else if (!Q_stricmp(token, "fresnelBias"))
		{
			ParseExpression(text, &stage->fresnelBiasExp);
		}
		// normalScale <arithmetic expression>
		else if (!Q_stricmp(token, "normalScale"))
		{
			ParseExpression(text, &stage->normalScaleExp);
		}
		// fogDensity <arithmetic expression>
		else if (!Q_stricmp(token, "fogDensity"))
		{
			ParseExpression(text, &stage->fogDensityExp);
		}
		// depthScale <arithmetic expression>
		else if (!Q_stricmp(token, "depthScale"))
		{
			ParseExpression(text, &stage->depthScaleExp);
		}
		// deformMagnitude <arithmetic expression>
		else if (!Q_stricmp(token, "deformMagnitude"))
		{
			ParseExpression(text, &stage->deformMagnitudeExp);
		}
		// blurMagnitude <arithmetic expression>
		else if (!Q_stricmp(token, "blurMagnitude"))
		{
			ParseExpression(text, &stage->blurMagnitudeExp);
		}
		// wrapAroundLighting <arithmetic expression>
		else if (!Q_stricmp(token, "wrapAroundLighting"))
		{
			ParseExpression(text, &stage->wrapAroundLightingExp);
		}
		// fragmentProgram <prog>
		else if (!Q_stricmp(token, "fragmentProgram"))
		{
			Ren_Warning("WARNING: fragmentProgram keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// vertexProgram <prog>
		else if (!Q_stricmp(token, "vertexProgram"))
		{
			Ren_Warning("WARNING: vertexProgram keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// program <prog>
		else if (!Q_stricmp(token, "program"))
		{
			Ren_Warning("WARNING: program keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// vertexParm <index> <exp0> [,exp1] [,exp2] [,exp3]
		else if (!Q_stricmp(token, "vertexParm"))
		{
			Ren_Warning("WARNING: vertexParm keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// fragmentMap <index> [options] <map>
		else if (!Q_stricmp(token, "fragmentMap"))
		{
			Ren_Warning("WARNING: fragmentMap keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// megaTexture <mega>
		else if (!Q_stricmp(token, "megaTexture"))
		{
			Ren_Warning("WARNING: megaTexture keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
		}
		// glowStage
		else if (!Q_stricmp(token, "glowStage"))
		{
			Ren_Warning("WARNING: glowStage keyword not supported in shader '%s'\n", shader.name);
		}
		else
		{
			Ren_Warning("WARNING: unknown shader stage parameter '%s' in shader '%s'\n", token, shader.name);
			SkipRestOfLine(text);
			return qfalse;
		}
	}

	// parsing succeeded
	stage->active = qtrue;

	// if cgen isn't explicitly specified, use either identity or identitylighting
	if (stage->rgbGen == CGEN_BAD)
	{
		if (blendSrcBits == 0 || blendSrcBits == GLS_SRCBLEND_ONE || blendSrcBits == GLS_SRCBLEND_SRC_ALPHA)
		{
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		else
		{
			stage->rgbGen = CGEN_IDENTITY;
		}
	}

	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ZERO))
	{
		blendDstBits  = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// tell shader if this stage has an alpha test
	if (atestBits & GLS_ATEST_BITS)
	{
		shader.alphaTest = qtrue;
	}

	// compute state bits
	stage->stateBits = colorMaskBits | depthMaskBits | blendSrcBits | blendDstBits | atestBits | depthFuncBits | polyModeBits;

	// load image
	if (loadMap && !LoadMap(stage, buffer))
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform(char **text)
{
	char          *token;
	deformStage_t *ds;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing deform parm in shader '%s'\n", shader.name);
		return;
	}

	if (shader.numDeforms == MAX_SHADER_DEFORMS)
	{
		Ren_Warning("WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name);
		return;
	}

	ds = &shader.deforms[shader.numDeforms];
	shader.numDeforms++;

	if (!Q_stricmp(token, "projectionShadow"))
	{
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if (!Q_stricmp(token, "autosprite"))
	{
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if (!Q_stricmp(token, "autosprite2"))
	{
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if (!Q_stricmpn(token, "text", 4))
	{
		int n;

		n = token[4] - '0';
		if (n < 0 || n > 7)
		{
			n = 0;
		}
		ds->deformation = (deform_t)(DEFORM_TEXT0 + n);
		return;
	}

	if (!Q_stricmp(token, "bulge"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeWidth = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeHeight = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeSpeed = atof(token);

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if (!Q_stricmp(token, "wave"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}

		if (atof(token) != 0)
		{
			ds->deformationSpread = 1.0f / atof(token);
		}
		else
		{
			ds->deformationSpread = 100.0f;
			Ren_Warning("WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if (!Q_stricmp(token, "normal"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.amplitude = atof(token);

		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.frequency = atof(token);

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if (!Q_stricmp(token, "move"))
	{
		int i;

		for (i = 0; i < 3; i++)
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
				return;
			}
			ds->moveVector[i] = atof(token);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_MOVE;
		return;
	}

	if (!Q_stricmp(token, "sprite"))
	{
		ds->deformation = DEFORM_SPRITE;
		return;
	}

	if (!Q_stricmp(token, "flare"))
	{
		token = COM_ParseExt2(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes flare parm in shader '%s'\n", shader.name);
			return;
		}
		ds->flareSize = atof(token);
		return;
	}

	Ren_Warning("WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name);
}

/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms(char **text)
{
	char *token;
	char prefix[MAX_QPATH];

	// outerbox
	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (strcmp(token, "-"))
	{
		Q_strncpyz(prefix, token, sizeof(prefix));

		shader.sky.outerbox = R_FindCubeImage(prefix, IF_NONE, FT_DEFAULT, WT_EDGE_CLAMP, shader.name);
		if (!shader.sky.outerbox)
		{
			Ren_Warning("WARNING: could not find cubemap '%s' for outer skybox in shader '%s'\n", prefix, shader.name);
			shader.sky.outerbox = tr.blackCubeImage;
		}
	}

	// cloudheight
	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	shader.sky.cloudHeight = atof(token);
	if (!shader.sky.cloudHeight)
	{
		shader.sky.cloudHeight = 512;
	}

	R_InitSkyTexCoords(shader.sky.cloudHeight);

	// innerbox
	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (strcmp(token, "-"))
	{
		Q_strncpyz(prefix, token, sizeof(prefix));

		shader.sky.innerbox = R_FindCubeImage(prefix, IF_NONE, FT_DEFAULT, WT_EDGE_CLAMP, shader.name);
		if (!shader.sky.innerbox)
		{
			Ren_Warning("WARNING: could not find cubemap '%s' for inner skybox in shader '%s'\n", prefix, shader.name);
			shader.sky.innerbox = tr.blackCubeImage;
		}
	}

	shader.isSky = qtrue;
}

/*
=================
ParseSort
=================
*/
static void ParseSort(char **text)
{
	char *token;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing sort parameter in shader '%s'\n", shader.name);
		return;
	}

	if (!Q_stricmp(token, "portal") || !Q_stricmp(token, "subview"))
	{
		shader.sort = SS_PORTAL;
	}
	else if (!Q_stricmp(token, "sky") || !Q_stricmp(token, "environment"))
	{
		shader.sort = SS_ENVIRONMENT_FOG;
	}
	else if (!Q_stricmp(token, "opaque"))
	{
		shader.sort = SS_OPAQUE;
	}
	else if (!Q_stricmp(token, "decal"))
	{
		shader.sort = SS_DECAL;
	}
	else if (!Q_stricmp(token, "seeThrough"))
	{
		shader.sort = SS_SEE_THROUGH;
	}
	else if (!Q_stricmp(token, "banner"))
	{
		shader.sort = SS_BANNER;
	}
	else if (!Q_stricmp(token, "underwater"))
	{
		shader.sort = SS_UNDERWATER;
	}
	else if (!Q_stricmp(token, "far"))
	{
		shader.sort = SS_FAR;
	}
	else if (!Q_stricmp(token, "medium"))
	{
		shader.sort = SS_MEDIUM;
	}
	else if (!Q_stricmp(token, "close"))
	{
		shader.sort = SS_CLOSE;
	}
	else if (!Q_stricmp(token, "additive"))
	{
		shader.sort = SS_BLEND1;
	}
	else if (!Q_stricmp(token, "almostNearest"))
	{
		shader.sort = SS_ALMOST_NEAREST;
	}
	else if (!Q_stricmp(token, "nearest"))
	{
		shader.sort = SS_NEAREST;
	}
	else if (!Q_stricmp(token, "postProcess"))
	{
		shader.sort = SS_POST_PROCESS;
	}
	else
	{
		shader.sort = atof(token);
	}
}

// this table is also present in xmap

typedef struct
{
	char *name;
	int clearSolid, surfaceFlags, contents;
} infoParm_t;

// *INDENT-OFF*
infoParm_t infoParms[] =
{
	// server relevant contents

	{ "clipmissile",        1, 0,                 CONTENTS_MISSILECLIP      }, // impact only specific weapons (rl, gl)

	{ "water",              1, 0,                 CONTENTS_WATER            },

	{ "slag",               1, 0,                 CONTENTS_SLIME            }, // uses the CONTENTS_SLIME flag, but the shader reference is changed to 'slag'
	// to idendify that this doesn't work the same as 'slime' did.

	{ "lava",               1, 0,                 CONTENTS_LAVA             }, // very damaging
	{ "playerclip",         1, 0,                 CONTENTS_PLAYERCLIP       },
	{ "monsterclip",        1, 0,                 CONTENTS_MONSTERCLIP      },

	{ "nodrop",             1, 0,                 CONTENTS_NODROP           }, // don't drop items or leave bodies (death fog, lava, etc)
	{ "nonsolid",           1, SURF_NONSOLID,     0                         }, // clears the solid flag

	{ "blood",              1, 0,                 CONTENTS_WATER            },

	// utility relevant attributes
	{ "origin",             1, 0,                 CONTENTS_ORIGIN           }, // center of rotating brushes

	{ "trans",              0, 0,                 CONTENTS_TRANSLUCENT      }, // don't eat contained surfaces
	{ "translucent",        0, 0,                 CONTENTS_TRANSLUCENT      }, // don't eat contained surfaces

	{ "detail",             0, 0,                 CONTENTS_DETAIL           }, // don't include in structural bsp
	{ "structural",         0, 0,                 CONTENTS_STRUCTURAL       }, // force into structural bsp even if trnas
	{ "areaportal",         1, 0,                 CONTENTS_AREAPORTAL       }, // divides areas
	{ "clusterportal",      1, 0,                 CONTENTS_CLUSTERPORTAL    }, // for bots
	{ "donotenter",         1, 0,                 CONTENTS_DONOTENTER       }, // for bots

	{ "donotenterlarge",    1, 0,                 CONTENTS_DONOTENTER_LARGE }, // for larger bots

	{ "fog",                1, 0,                 CONTENTS_FOG              }, // carves surfaces entering
	{ "sky",                0, SURF_SKY,          0                         }, // emit light from an environment map
	{ "lightfilter",        0, SURF_LIGHTFILTER,  0                         }, // filter light going through it
	{ "alphashadow",        0, SURF_ALPHASHADOW,  0                         }, // test light on a per-pixel basis
	{ "hint",               0, SURF_HINT,         0                         }, // use as a primary splitter

	// server attributes
	{ "slick",              0, SURF_SLICK,        0                         },

	{ "noimpact",           0, SURF_NOIMPACT,     0                         }, // don't make impact explosions or marks

	{ "nomarks",            0, SURF_NOMARKS,      0                         }, // don't make impact marks, but still explode
	{ "nooverlays",         0, SURF_NOMARKS,      0                         }, // don't make impact marks, but still explode

	{ "ladder",             0, SURF_LADDER,       0                         },

	{ "nodamage",           0, SURF_NODAMAGE,     0                         },

	{ "monsterslick",       0, SURF_MONSTERSLICK, 0                         }, // surf only slick for monsters

	{ "glass",              0, SURF_GLASS,        0                         },
	{ "splash",             0, SURF_SPLASH,       0                         },

	// steps
	{ "metal",              0, SURF_METAL,        0                         },
	{ "metalsteps",         0, SURF_METAL,        0                         },

	{ "nosteps",            0, SURF_NOSTEPS,      0                         },
	{ "woodsteps",          0, SURF_WOOD,         0                         },
	{ "grasssteps",         0, SURF_GRASS,        0                         },
	{ "gravelsteps",        0, SURF_GRAVEL,       0                         },
	{ "carpetsteps",        0, SURF_CARPET,       0                         },
	{ "snowsteps",          0, SURF_SNOW,         0                         },
	{ "roofsteps",          0, SURF_ROOF,         0                         }, // tile roof

	{ "rubble",             0, SURF_RUBBLE,       0                         },

	// drawsurf attributes
	{ "nodraw",             0, SURF_NODRAW,       0                         }, // don't generate a drawsurface (or a lightmap)
	{ "pointlight",         0, SURF_POINTLIGHT,   0                         }, // sample lighting at vertexes
	{ "nolightmap",         0, SURF_NOLIGHTMAP,   0                         }, // don't generate a lightmap
	{ "nodlight",           0, 0,                 0                         }, // OBSELETE: don't ever add dynamic lights

	// monster ai
	{ "monsterslicknorth",  0, SURF_MONSLICK_N,   0                         },
	{ "monsterslickeast",   0, SURF_MONSLICK_E,   0                         },
	{ "monsterslicksouth",  0, SURF_MONSLICK_S,   0                         },
	{ "monsterslickwest",   0, SURF_MONSLICK_W,   0                         },

	// unsupported Doom3 surface types for sound effects and blood splats
	{ "metal",              0, SURF_METAL,        0                         },

	{ "stone",              0, 0,                 0                         },
	{ "wood",               0, SURF_WOOD,         0                         },
	{ "cardboard",          0, 0,                 0                         },
	{ "liquid",             0, 0,                 0                         },
	{ "glass",              0, 0,                 0                         },
	{ "plastic",            0, 0,                 0                         },
	{ "ricochet",           0, 0,                 0                         },
	{ "surftype10",         0, 0,                 0                         },
	{ "surftype11",         0, 0,                 0                         },
	{ "surftype12",         0, 0,                 0                         },
	{ "surftype13",         0, 0,                 0                         },
	{ "surftype14",         0, 0,                 0                         },
	{ "surftype15",         0, 0,                 0                         },

	// other unsupported Doom3 surface types
	{ "trigger",            0, 0,                 0                         },
	{ "flashlight_trigger", 0, 0,                 0                         },
	{ "aassolid",           0, 0,                 0                         },
	{ "aasobstacle",        0, 0,                 0                         },
	{ "nullNormal",         0, 0,                 0                         },
	{ "discrete",           0, 0,                 0                         },

};
// *INDENT-ON*

/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static qboolean SurfaceParm(const char *token)
{
	int numInfoParms = sizeof(infoParms) / sizeof(infoParms[0]);
	int i;

	for (i = 0; i < numInfoParms; i++)
	{
		if (!Q_stricmp(token, infoParms[i].name))
		{
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
#if 0
			if (infoParms[i].clearSolid)
			{
				si->contents &= ~CONTENTS_SOLID;
			}
#endif
			return qtrue;
		}
	}

	return qfalse;
}

static void ParseSurfaceParm(char **text)
{
	char *token;

	token = COM_ParseExt2(text, qfalse);
	SurfaceParm(token);
}

static void ParseDiffuseMap(shaderStage_t *stage, char **text)
{
	char buffer[1024] = "";

	stage->active    = qtrue;
	stage->type      = ST_DIFFUSEMAP;
	stage->rgbGen    = CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;
	if (!r_compressDiffuseMaps->integer)
	{
		stage->forceHighQuality = qtrue;
	}

	if (ParseMap(stage, text, buffer, sizeof(buffer)))
	{
		LoadMap(stage, buffer);
	}
}

static void ParseNormalMap(shaderStage_t *stage, char **text)
{
	char buffer[1024] = "";

	stage->active    = qtrue;
	stage->type      = ST_NORMALMAP;
	stage->rgbGen    = CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;
	if (!r_compressNormalMaps->integer)
	{
		stage->forceHighQuality = qtrue;
	}
	if (r_highQualityNormalMapping->integer)
	{
		stage->overrideFilterType = qtrue;
		stage->filterType         = FT_LINEAR;

		stage->overrideNoPicMip = qtrue;
	}

	if (ParseMap(stage, text, buffer, sizeof(buffer)))
	{
		LoadMap(stage, buffer);
	}
}

static void ParseSpecularMap(shaderStage_t *stage, char **text)
{
	char buffer[1024] = "";

	stage->active    = qtrue;
	stage->type      = ST_SPECULARMAP;
	stage->rgbGen    = CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;
	if (!r_compressSpecularMaps->integer)
	{
		stage->forceHighQuality = qtrue;
	}

	if (ParseMap(stage, text, buffer, sizeof(buffer)))
	{
		LoadMap(stage, buffer);
	}
}

static void ParseGlowMap(shaderStage_t *stage, char **text)
{
	char buffer[1024] = "";

	stage->active    = qtrue;
	stage->type      = ST_COLORMAP;
	stage->rgbGen    = CGEN_IDENTITY;
	stage->stateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE; // blend add

	if (ParseMap(stage, text, buffer, sizeof(buffer)))
	{
		LoadMap(stage, buffer);
	}
}

static void ParseReflectionMap(shaderStage_t *stage, char **text)
{
	char buffer[1024] = "";

	stage->active           = qtrue;
	stage->type             = ST_REFLECTIONMAP;
	stage->rgbGen           = CGEN_IDENTITY;
	stage->stateBits        = GLS_DEFAULT;
	stage->overrideWrapType = qtrue;
	stage->wrapType         = WT_EDGE_CLAMP;

	if (ParseMap(stage, text, buffer, sizeof(buffer)))
	{
		LoadMap(stage, buffer);
	}
}

static void ParseReflectionMapBlended(shaderStage_t *stage, char **text)
{
	char buffer[1024] = "";

	stage->active           = qtrue;
	stage->type             = ST_REFLECTIONMAP;
	stage->rgbGen           = CGEN_IDENTITY;
	stage->stateBits        = GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE;
	stage->overrideWrapType = qtrue;
	stage->wrapType         = WT_EDGE_CLAMP;

	if (ParseMap(stage, text, buffer, sizeof(buffer)))
	{
		LoadMap(stage, buffer);
	}
}

static void ParseLightFalloffImage(shaderStage_t *stage, char **text)
{
	char buffer[1024] = "";

	stage->active           = qtrue;
	stage->type             = ST_ATTENUATIONMAP_Z;
	stage->rgbGen           = CGEN_IDENTITY;
	stage->stateBits        = GLS_DEFAULT;
	stage->overrideWrapType = qtrue;
	stage->wrapType         = WT_EDGE_CLAMP;

	if (ParseMap(stage, text, buffer, sizeof(buffer)))
	{
		LoadMap(stage, buffer);
	}
}

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader template files for
the given guide name.

return NULL if not found

If found, it will return a valid template
=====================
*/
static char *FindGuideInGuideText(const char *guideName)
{
	char *token, *p;

	int i, hash;

	if (!s_guideText)
	{
		// no guides loaded at all
		return NULL;
	}

	hash = generateHashValue(guideName, MAX_GUIDETEXT_HASH);

	for (i = 0; guideTextHashTable[hash][i]; i++)
	{
		p     = guideTextHashTable[hash][i];
		token = COM_ParseExt2(&p, qtrue);
		if (!Q_stricmp(token, guideName))
		{
			//Ren_Print("found guide '%s' by hashing\n", guideName);
			return p;
		}
	}

	p = s_guideText;

	if (!p)
	{
		return NULL;
	}

	// look for label
	while (1)
	{
		token = COM_ParseExt2(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		if (Q_stricmp(token, "guide") && Q_stricmp(token, "inlineGuide"))
		{
			Ren_Warning("WARNING: expected guide or inlineGuide found '%s'\n", token);
			break;
		}

		// parse guide name
		token = COM_ParseExt2(&p, qtrue);

		if (!Q_stricmp(token, guideName))
		{
			Ren_Print("found guide '%s' by linear search\n", guideName);
			return p;
		}

		// skip parameters
		token = COM_ParseExt2(&p, qtrue);
		if (Q_stricmp(token, "("))
		{
			Ren_Warning("WARNING: expected ( found '%s'\n", token);
			break;
		}

		while (1)
		{
			token = COM_ParseExt2(&p, qtrue);

			if (!token[0])
			{
				break;
			}

			if (!Q_stricmp(token, ")"))
			{
				break;
			}
		}

		if (Q_stricmp(token, ")"))
		{
			Ren_Warning("WARNING: expected ) found '%s'\n", token);
			break;
		}

		// skip guide body
		SkipBracedSection(&p);
	}

	return NULL;
}

/*
=================
CreateShaderByGuide
=================
*/
#define MAX_GUIDE_PARAMETERS 16
static char *CreateShaderByGuide(const char *guideName, char *shaderText)
{
	int         i;
	char        *guideText;
	char        *token;
	const char  *p;
	static char buffer[4096];
	char        name[MAX_QPATH];
	int         numGuideParms;
	char        guideParms[MAX_GUIDE_PARAMETERS][MAX_QPATH];
	int         numShaderParms;
	char        shaderParms[MAX_GUIDE_PARAMETERS][MAX_QPATH];

	Com_Memset(buffer, 0, sizeof(buffer));
	Com_Memset(guideParms, 0, sizeof(guideParms));
	Com_Memset(shaderParms, 0, sizeof(shaderParms));

	// attempt to define shader from an explicit parameter file
	guideText = FindGuideInGuideText(guideName);
	if (guideText)
	{
		shader.createdByGuide = qtrue;

		// parse guide parameters
		numGuideParms = 0;

		token = COM_ParseExt2(&guideText, qtrue);
		if (Q_stricmp(token, "("))
		{
			Ren_Warning("WARNING: expected ( found '%s'\n", token);
			return NULL;
		}

		while (1)
		{
			token = COM_ParseExt2(&guideText, qtrue);

			if (!token[0])
			{
				break;
			}

			if (!Q_stricmp(token, ")"))
			{
				break;
			}

			if (numGuideParms >= MAX_GUIDE_PARAMETERS - 1)
			{
				Ren_Print("WARNING: more than %i guide parameters are not allowed\n", MAX_GUIDE_PARAMETERS);
				return NULL;
			}

			//Ren_Print("guide parameter %i = '%s'\n", numGuideParms, token);

			Q_strncpyz(guideParms[numGuideParms], token, MAX_QPATH);
			numGuideParms++;
		}

		if (Q_stricmp(token, ")"))
		{
			Ren_Print("WARNING: expected ) found '%s'\n", token);
			return NULL;
		}

		// parse shader parameters
		numShaderParms = 0;

		token = COM_ParseExt2(&shaderText, qtrue);
		if (Q_stricmp(token, "("))
		{
			Ren_Print("WARNING: expected ( found '%s'\n", token);
			return NULL;
		}

		while (1)
		{
			token = COM_ParseExt2(&shaderText, qtrue);

			if (!token[0])
			{
				break;
			}

			if (!Q_stricmp(token, ")"))
			{
				break;
			}

			if (numShaderParms >= MAX_GUIDE_PARAMETERS - 1)
			{
				Ren_Print("WARNING: more than %i guide parameters are not allowed\n", MAX_GUIDE_PARAMETERS);
				return NULL;
			}

			//Ren_Print("shader parameter %i = '%s'\n", numShaderParms, token);

			Q_strncpyz(shaderParms[numShaderParms], token, MAX_QPATH);
			numShaderParms++;
		}

		if (Q_stricmp(token, ")"))
		{
			Ren_Print("WARNING: expected ) found '%s'\n", token);
			return NULL;
		}

		if (numGuideParms != numShaderParms)
		{
			Ren_Warning("WARNING: %i numGuideParameters != %i numShaderParameters\n", numGuideParms, numShaderParms);
			return NULL;
		}

#if 0
		for (i = 0; i < numGuideParms; i++)
		{
			Ren_Print("guide parameter '%s' = '%s'\n", guideParms[i], shaderParms[i]);
		}
#endif

		token = COM_ParseExt2(&guideText, qtrue);
		if (Q_stricmp(token, "{"))
		{
			Ren_Print("WARNING: expected { found '%s'\n", token);
			return NULL;
		}

		// create buffer
		while (1)
		{
			// begin new line
			token = COM_ParseExt2(&guideText, qtrue);

			if (!token[0])
			{
				Ren_Warning("WARNING: no concluding '}' in guide %s\n", guideName);
				return NULL;
			}

			// end of guide definition
			if (token[0] == '}')
			{
				break;
			}

			Q_strncpyz(name, token, sizeof(name));

#if 0
			// adjust name by guide parameters if necessary
			for (i = 0; i < numGuideParms; i++)
			{
				if ((p = Q_stristr(name, (const char *)guideParms)))
				{
					//Ren_Print("guide parameter '%s' = '%s'\n", guideParms[i], shaderParms[i]);

					Q_strreplace(name, sizeof(name), guideParms[i], shaderParms[i]);
				}
			}
#endif

			Q_strcat(buffer, sizeof(buffer), name);
			Q_strcat(buffer, sizeof(buffer), " ");

			// parse rest of line
			while (1)
			{
				token = COM_ParseExt2(&guideText, qfalse);

				if (!token[0])
				{
					// end of line
					break;
				}

				Q_strncpyz(name, token, sizeof(name));

				// adjust name by guide parameters if necessary
				for (i = 0; i < numGuideParms; i++)
				{
					if ((p = Q_stristr(name, (const char *)guideParms)))
					{
						//Ren_Print("guide parameter '%s' = '%s'\n", guideParms[i], shaderParms[i]);

						Q_strreplace(name, sizeof(name), guideParms[i], shaderParms[i]);
					}
				}

				Q_strcat(buffer, sizeof(buffer), name);
				Q_strcat(buffer, sizeof(buffer), " ");
			}

			Q_strcat(buffer, sizeof(buffer), "\n");
		}

		if (Q_stricmp(token, "}"))
		{
			Ren_Print("WARNING: expected } found '%s'\n", token);
			return NULL;
		}

		Q_strcat(buffer, sizeof(buffer), "}");

		Ren_Print("----- '%s' -----\n%s\n----------\n", shader.name, buffer);

		return buffer;
	}

	return NULL;
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader(char *_text)
{
	char **text = &_text;
	char *token;
	int  s = 0;

	shader.explicitlyDefined = qtrue;

	token = COM_ParseExt2(text, qtrue);
	if (token[0] != '{')
	{
		if (!(_text = CreateShaderByGuide(token, _text)))
		{
			Ren_Warning("WARNING: couldn't create shader '%s' by template '%s'\n", shader.name, token);
			//Ren_Warning( "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name);
			return qfalse;
		}
		else
		{
			text = &_text;
		}
	}

	while (1)
	{
		token = COM_ParseExt2(text, qtrue);
		if (!token[0])
		{
			Ren_Warning("WARNING: no concluding '}' in shader %s\n", shader.name);
			return qfalse;
		}

		// end of shader definition
		if (token[0] == '}')
		{
			break;
		}
		// stage definition
		else if (token[0] == '{')
		{
			if (s >= (MAX_SHADER_STAGES - 1))
			{
				Ren_Warning("WARNING: too many stages in shader %s\n", shader.name);
				return qfalse;
			}

			if (!ParseStage(&stages[s], text))
			{
				Ren_Warning("WARNING: can't parse stages of shader %s @[%.50s ...]\n", shader.name, _text);
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if (!Q_stricmpn(token, "qer", 3))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip description
		else if (!Q_stricmp(token, "description"))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip renderbump
		else if (!Q_stricmp(token, "renderbump"))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip unsmoothedTangents
		else if (!Q_stricmp(token, "unsmoothedTangents"))
		{
			Ren_Warning("WARNING: unsmoothedTangents keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// skip guiSurf
		else if (!Q_stricmp(token, "guiSurf"))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip decalInfo
		else if (!Q_stricmp(token, "decalInfo"))
		{
			Ren_Warning("WARNING: decalInfo keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			continue;
		}
		// skip Quake4's extra material types
		else if (!Q_stricmp(token, "materialType"))
		{
			Ren_Warning("WARNING: materialType keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			continue;
		}
		// skip Prey's extra material types
		else if (!Q_stricmpn(token, "matter", 6))
		{
			//Ren_Warning( "WARNING: materialType keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			continue;
		}
		// sun parms
		else if (!Q_stricmp(token, "xmap_sun") ||
		         !Q_stricmp(token, "q3map_sun"))
		{
			float a, b;

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			tr.sunLight[0] = atof(token);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			tr.sunLight[1] = atof(token);


			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			tr.sunLight[2] = atof(token);

			VectorNormalize(tr.sunLight);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			a = atof(token);
			VectorScale(tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			a = atof(token);
			a = a / 180 * M_PI;

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			b = atof(token);
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos(a) * cos(b);
			tr.sunDirection[1] = sin(a) * cos(b);
			tr.sunDirection[2] = sin(b);
			continue;
		}
		// noShadows
		else if (!Q_stricmp(token, "noShadows"))
		{
			shader.noShadows = qtrue;
			continue;
		}
		// noSelfShadow
		else if (!Q_stricmp(token, "noSelfShadow"))
		{
			Ren_Warning("WARNING: noSelfShadow keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// forceShadows
		else if (!Q_stricmp(token, "forceShadows"))
		{
			Ren_Warning("WARNING: forceShadows keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// forceOverlays
		else if (!Q_stricmp(token, "forceOverlays"))
		{
			Ren_Warning("WARNING: forceOverlays keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// noPortalFog
		else if (!Q_stricmp(token, "noPortalFog"))
		{
			Ren_Warning("WARNING: noPortalFog keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// fogLight
		else if (!Q_stricmp(token, "fogLight"))
		{
			Ren_Warning("WARNING: fogLight keyword not supported in shader '%s'\n", shader.name);
			shader.fogLight = qtrue;
			continue;
		}
		// blendLight
		else if (!Q_stricmp(token, "blendLight"))
		{
			Ren_Warning("WARNING: blendLight keyword not supported in shader '%s'\n", shader.name);
			shader.blendLight = qtrue;
			continue;
		}
		// ambientLight
		else if (!Q_stricmp(token, "ambientLight"))
		{
			Ren_Warning("WARNING: ambientLight keyword not supported in shader '%s'\n", shader.name);
			shader.ambientLight = qtrue;
			continue;
		}
		// volumetricLight
		else if (!Q_stricmp(token, "volumetricLight"))
		{
			shader.volumetricLight = qtrue;
			continue;
		}
		// translucent
		else if (!Q_stricmp(token, "translucent"))
		{
			shader.translucent = qtrue;
			continue;
		}
		// forceOpaque
		else if (!Q_stricmp(token, "forceOpaque"))
		{
			shader.forceOpaque = qtrue;
			continue;
		}
		// forceSolid
		else if (!Q_stricmp(token, "forceSolid") || !Q_stricmp(token, "solid"))
		{
			continue;
		}
		else if (!Q_stricmp(token, "deformVertexes") || !Q_stricmp(token, "deform"))
		{
			ParseDeform(text);
			continue;
		}
		else if (!Q_stricmp(token, "tesssize"))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip noFragment
		if (!Q_stricmp(token, "noFragment"))
		{
			continue;
		}
		// skip stuff that only the xmap needs
		else if (!Q_stricmpn(token, "xmap", 4) || !Q_stricmpn(token, "q3map", 5))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip stuff that only xmap or the server needs
		else if (!Q_stricmp(token, "surfaceParm"))
		{
			ParseSurfaceParm(text);
			continue;
		}
		// no mip maps
		else if (!Q_stricmp(token, "nomipmap") || !Q_stricmp(token, "nomipmaps"))
		{
			shader.filterType = FT_LINEAR;
			shader.noPicMip   = qtrue;
			continue;
		}
		// no picmip adjustment
		else if (!Q_stricmp(token, "nopicmip"))
		{
			shader.noPicMip = qtrue;
			continue;
		}
		// RF, allow each shader to permit compression if available
		else if (!Q_stricmp(token, "allowcompress"))
		{
			shader.uncompressed = qfalse;
			continue;
		}
		else if (!Q_stricmp(token, "nocompress"))
		{
			shader.uncompressed = qtrue;
			continue;
		}
		// polygonOffset
		else if (!Q_stricmp(token, "polygonOffset"))
		{
			shader.polygonOffset = qtrue;

			token = COM_ParseExt2(text, qfalse);
			if (token[0])
			{
				shader.polygonOffsetValue = atof(token);
			}
			continue;
		}
		// parallax mapping
		else if (!Q_stricmp(token, "parallax"))
		{
			shader.parallax = qtrue;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if (!Q_stricmp(token, "entityMergable"))
		{
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if (!Q_stricmp(token, "fogParms"))
		{
			/*
			Ren_Warning( "WARNING: fogParms keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);

			*/

			if (!ParseVector(text, 3, shader.fogParms.color))
			{
				return qfalse;
			}

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name);
				continue;
			}
			shader.fogParms.depthForOpaque = atof(token);

			shader.fogVolume = qtrue;
			shader.sort      = SS_FOG;

			// skip any old gradient directions
			SkipRestOfLine(text);
			continue;
		}
		// noFog
		else if (!Q_stricmp(token, "noFog"))
		{
			shader.noFog = qtrue;
			continue;
		}
		// portal
		else if (!Q_stricmp(token, "portal"))
		{
			shader.sort     = SS_PORTAL;
			shader.isPortal = qtrue;

			token = COM_ParseExt2(text, qfalse);
			if (token[0])
			{
				shader.portalRange = atof(token);
			}
			else
			{
				shader.portalRange = 256;
			}
			continue;
		}
		// portal or mirror
		else if (!Q_stricmp(token, "mirror"))
		{
			shader.sort     = SS_PORTAL;
			shader.isPortal = qtrue;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if (!Q_stricmp(token, "skyparms"))
		{
			ParseSkyParms(text);
			continue;
		}
		// This is fixed fog for the skybox/clouds determined solely by the shader
		// it will not change in a level and will not be necessary
		// to force clients to use a sky fog the server says to.
		// skyfogvars <(r,g,b)> <dist>
		else if (!Q_stricmp(token, "skyfogvars"))
		{
			vec3_t fogColor;

			if (!ParseVector(text, 3, fogColor))
			{
				return qfalse;
			}
			token = COM_ParseExt(text, qfalse);

			if (!token[0])
			{
				Ren_Warning("WARNING: missing density value for sky fog\n");
				continue;
			}

			if (atof(token) > 1)
			{
				Ren_Warning("WARNING: last value for skyfogvars is 'density' which needs to be 0.0-1.0\n");
				continue;
			}

			RE_SetFog(FOG_SKY, 0, 5, fogColor[0], fogColor[1], fogColor[2], atof(token));

			continue;
		}
		// ET waterfogvars
		else if (!Q_stricmp(token, "waterfogvars"))
		{
			vec3_t watercolor;
			float  fogvar;

			if (!ParseVector(text, 3, watercolor))
			{
				return qfalse;
			}
			token = COM_ParseExt(text, qfalse);

			if (!token[0])
			{
				Ren_Warning("WARNING: missing density/distance value for water fog\n");
				continue;
			}

			fogvar = atof(token);

			// right now allow one water color per map.  I'm sure this will need
			//          to change at some point, but I'm not sure how to track fog parameters
			//          on a "per-water volume" basis yet.
			if (fogvar == 0)
			{                   // '0' specifies "use the map values for everything except the fog color
				// TODO
			}
			else if (fogvar > 1)
			{                   // distance "linear" fog
				RE_SetFog(FOG_WATER, 0, fogvar, watercolor[0], watercolor[1], watercolor[2], 1.1);
			}
			else
			{                   // density "exp" fog
				RE_SetFog(FOG_WATER, 0, 5, watercolor[0], watercolor[1], watercolor[2], fogvar);
			}
			continue;
		}
		// ET fogvars
		else if (!Q_stricmp(token, "fogvars"))
		{
			vec3_t fogColor;
			float  fogDensity;
			int    fogFar;

			if (!ParseVector(text, 3, fogColor))
			{
				return qfalse;
			}

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing density value for the fog\n");
				continue;
			}

			// NOTE:   fogFar > 1 means the shader is setting the farclip, < 1 means setting
			//         density (so old maps or maps that just need softening fog don't have to care about farclip)

			fogDensity = atof(token);
			if (fogDensity > 1)
			{                   // linear
				fogFar = fogDensity;
			}
			else
			{
				fogFar = 5;
			}

			RE_SetFog(FOG_MAP, 0, fogFar, fogColor[0], fogColor[1], fogColor[2], fogDensity);
			RE_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 50, 0, 0, 0, 0);
			continue;
		}
		// ET sunshader <name>
		else if (!Q_stricmp(token, "sunshader"))
		{
			int tokenLen;

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing shader name for 'sunshader'\n");
				continue;
			}

			/*
			RB: don't call tr.sunShader = R_FindShader(token, SHADER_3D_STATIC, qtrue);
			    because it breaks the computation of the current shader
			*/
			tokenLen         = strlen(token) + 1;
			tr.sunShaderName = (char *)ri.Hunk_Alloc(sizeof(char) * tokenLen, h_low);
			Q_strncpyz(tr.sunShaderName, token, tokenLen);
		}
		else if (!Q_stricmp(token, "lightgridmulamb"))
		{
			// ambient multiplier for lightgrid
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing value for 'lightgrid ambient multiplier'\n");
				continue;
			}
			if (atof(token) > 0)
			{
				tr.lightGridMulAmbient = atof(token);
			}
		}
		else if (!Q_stricmp(token, "lightgridmuldir"))
		{
			// directional multiplier for lightgrid
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing value for 'lightgrid directional multiplier'\n");
				continue;
			}
			if (atof(token) > 0)
			{
				tr.lightGridMulDirected = atof(token);
			}
		}
		// light <value> determines flaring in xmap, not needed here
		else if (!Q_stricmp(token, "light"))
		{
			token = COM_ParseExt2(text, qfalse);
			continue;
		}
		// cull <face>
		else if (!Q_stricmp(token, "cull"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing cull parms in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "none") || !Q_stricmp(token, "twoSided") || !Q_stricmp(token, "disable"))
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if (!Q_stricmp(token, "back") || !Q_stricmp(token, "backside") || !Q_stricmp(token, "backsided"))
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				Ren_Warning("WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name);
			}
			continue;
		}
		// distancecull <opaque distance> <transparent distance> <alpha threshold>
		else if (!Q_stricmp(token, "distancecull"))
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					Ren_Warning("WARNING: missing distancecull parms in shader '%s'\n", shader.name);
				}
				else
				{
					shader.distanceCull[i] = atof(token);
				}
			}

			if (shader.distanceCull[1] - shader.distanceCull[0] > 0)
			{
				// distanceCull[ 3 ] is an optimization
				shader.distanceCull[3] = 1.0 / (shader.distanceCull[1] - shader.distanceCull[0]);
			}
			else
			{
				shader.distanceCull[0] = 0;
				shader.distanceCull[1] = 0;
				shader.distanceCull[2] = 0;
				shader.distanceCull[3] = 0;
			}
			continue;
		}
		// twoSided
		else if (!Q_stricmp(token, "twoSided"))
		{
			shader.cullType = CT_TWO_SIDED;
			continue;
		}
		// backSided
		else if (!Q_stricmp(token, "backSided"))
		{
			shader.cullType = CT_BACK_SIDED;
			continue;
		}
		// clamp
		else if (!Q_stricmp(token, "clamp"))
		{
			shader.wrapType = WT_CLAMP;
			continue;
		}
		// edgeClamp
		else if (!Q_stricmp(token, "edgeClamp"))
		{
			shader.wrapType = WT_EDGE_CLAMP;
			continue;
		}
		// zeroClamp
		else if (!Q_stricmp(token, "zeroclamp"))
		{
			shader.wrapType = WT_ZERO_CLAMP;
			continue;
		}
		// alphaZeroClamp
		else if (!Q_stricmp(token, "alphaZeroClamp"))
		{
			shader.wrapType = WT_ALPHA_ZERO_CLAMP;
			continue;
		}
		// sort
		else if (!Q_stricmp(token, "sort"))
		{
			ParseSort(text);
			continue;
		}
		// implicit default mapping to eliminate redundant/incorrect explicit shader stages
		else if (!Q_stricmpn(token, "implicit", 8))
		{
			//Ren_Warning( "WARNING: keyword '%s' not supported in shader '%s'\n", token, shader.name);
			//SkipRestOfLine(text);

			// set implicit mapping state
			if (!Q_stricmp(token, "implicitBlend"))
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				implicitCullType  = CT_TWO_SIDED;
			}
			else if (!Q_stricmp(token, "implicitMask"))
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_128;
				implicitCullType  = CT_TWO_SIDED;
			}
			else                // "implicitMap"
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE;
				implicitCullType  = CT_FRONT_SIDED;
			}

			// get image
			token = COM_ParseExt(text, qfalse);
			if (token[0] != '\0')
			{
				Q_strncpyz(implicitMap, token, sizeof(implicitMap));
			}
			else
			{
				implicitMap[0] = '-';
				implicitMap[1] = '\0';
			}

			continue;
		}
		// spectrum
		else if (!Q_stricmp(token, "spectrum"))
		{
			Ren_Warning("WARNING: spectrum keyword not supported in shader '%s'\n", shader.name);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'spectrum' keyword in shader '%s'\n", shader.name);
				continue;
			}
			shader.spectrum      = qtrue;
			shader.spectrumValue = atoi(token);
			continue;
		}
		// diffuseMap <image>
		else if (!Q_stricmp(token, "diffuseMap"))
		{
			ParseDiffuseMap(&stages[s], text);
			s++;
			continue;
		}
		// normalMap <image>
		else if (!Q_stricmp(token, "normalMap") || !Q_stricmp(token, "bumpMap"))
		{
			ParseNormalMap(&stages[s], text);
			s++;
			continue;
		}
		// specularMap <image>
		else if (!Q_stricmp(token, "specularMap"))
		{
			ParseSpecularMap(&stages[s], text);
			s++;
			continue;
		}
		// glowMap <image>
		else if (!Q_stricmp(token, "glowMap"))
		{
			ParseGlowMap(&stages[s], text);
			s++;
			continue;
		}
		// reflectionMap <image>
		else if (!Q_stricmp(token, "reflectionMap"))
		{
			ParseReflectionMap(&stages[s], text);
			s++;
			continue;
		}
		// reflectionMapBlended <image>
		else if (!Q_stricmp(token, "reflectionMapBlended"))
		{
			ParseReflectionMapBlended(&stages[s], text);
			s++;
			continue;
		}
		// lightMap <image>
		else if (!Q_stricmp(token, "lightMap"))
		{
			Ren_Warning("WARNING: obsolete lightMap keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			continue;
		}
		// lightFalloffImage <image>
		else if (!Q_stricmp(token, "lightFalloffImage"))
		{
			ParseLightFalloffImage(&stages[s], text);
			s++;
			continue;
		}
		// Doom 3 DECAL_MACRO
		else if (!Q_stricmp(token, "DECAL_MACRO"))
		{
			shader.polygonOffset      = qtrue;
			shader.polygonOffsetValue = 1;
			shader.sort               = SS_DECAL;
			SurfaceParm("discrete");
			SurfaceParm("noShadows");
			continue;
		}
		// Prey DECAL_ALPHATEST_MACRO
		else if (!Q_stricmp(token, "DECAL_ALPHATEST_MACRO"))
		{
			// what's different?
			shader.polygonOffset      = qtrue;
			shader.polygonOffsetValue = 1;
			shader.sort               = SS_DECAL;
			SurfaceParm("discrete");
			SurfaceParm("noShadows");
			continue;
		}
		else if (SurfaceParm(token))
		{
			continue;
		}
		else
		{
			Ren_Warning("WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name);
			return qfalse;
		}
	}

	// ignore shaders that don't have any stages, unless it is a sky or fog
	if (s == 0 && !shader.forceOpaque && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG) && implicitMap[0] == '\0')
	{
		return qfalse;
	}

	return qtrue;
}

/*
========================================================================================
SHADER OPTIMIZATION AND FOGGING
========================================================================================
*/

/*
typedef struct
{
    int             blendA;
    int             blendB;

    int             multitextureEnv;
    int             multitextureBlend;
} collapse_t;

static collapse_t collapse[] = {
    {0, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
     GL_MODULATE, 0},

    {0, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
     GL_MODULATE, 0},

    {GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
     GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR},

    {GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
     GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR},

    {GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
     GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR},

    {GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
     GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR},

    {0, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
     GL_ADD, 0},

    {GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
     GL_ADD, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE},
#if 0
    {0, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
     GL_DECAL, 0},
#endif
    {-1}
};
*/

/*
================
CollapseMultitexture
=================
*/
// *INDENT-OFF*
static void CollapseStages()
{
//	int             abits, bbits;
	int i, j;

	qboolean hasDiffuseStage;
	qboolean hasNormalStage;
	qboolean hasSpecularStage;
	qboolean hasReflectionStage;

	shaderStage_t tmpDiffuseStage;
	shaderStage_t tmpNormalStage;
	shaderStage_t tmpSpecularStage;
	shaderStage_t tmpReflectionStage;

	//int           idxColorStage;
	shaderStage_t tmpColorStage;

	//int           idxLightmapStage;
	shaderStage_t tmpLightmapStage;

	shader_t tmpShader;

	int           numStages = 0;
	shaderStage_t tmpStages[MAX_SHADER_STAGES];

	if (!r_collapseStages->integer)
	{
		return;
	}

	//Ren_Print("...collapsing '%s'\n", shader.name);

	Com_Memcpy(&tmpShader, &shader, sizeof(shader));

	Com_Memset(&tmpStages[0], 0, sizeof(stages));
	//Com_Memcpy(&tmpStages[0], &stages[0], sizeof(stages));

	for (j = 0; j < MAX_SHADER_STAGES; j++)
	{
		hasDiffuseStage    = qfalse;
		hasNormalStage     = qfalse;
		hasSpecularStage   = qfalse;
		hasReflectionStage = qfalse;

		Com_Memset(&tmpDiffuseStage, 0, sizeof(shaderStage_t));
		Com_Memset(&tmpNormalStage, 0, sizeof(shaderStage_t));
		Com_Memset(&tmpSpecularStage, 0, sizeof(shaderStage_t));

		//idxColorStage = -1;
		Com_Memset(&tmpColorStage, 0, sizeof(shaderStage_t));

		//idxLightmapStage = -1;
		Com_Memset(&tmpLightmapStage, 0, sizeof(shaderStage_t));

		if (!stages[j].active)
		{
			continue;
		}

		if (stages[j].type == ST_REFRACTIONMAP ||
		    stages[j].type == ST_DISPERSIONMAP ||
		    stages[j].type == ST_SKYBOXMAP ||
		    stages[j].type == ST_SCREENMAP ||
		    stages[j].type == ST_PORTALMAP ||
		    stages[j].type == ST_HEATHAZEMAP ||
		    stages[j].type == ST_LIQUIDMAP ||
		    stages[j].type == ST_ATTENUATIONMAP_XY ||
		    stages[j].type == ST_ATTENUATIONMAP_Z)
		{
			// only merge lighting relevant stages
			tmpStages[numStages] = stages[j];
			numStages++;
			continue;
		}

#if 0 //defined(COMPAT_Q3A) || defined(COMPAT_ET)
		for (i = 0; i < 2; i++)
		{
			if ((j + i) >= MAX_SHADER_STAGES)
			{
				continue;
			}

			if (!stages[j + i].active)
			{
				continue;
			}

			if (stages[j + i].type == ST_COLORMAP && idxColorStage == -1)
			{
				idxColorStage = j + i;
				tmpColorStage = stages[j + i];
			}
			else if (stages[j + i].type == ST_LIGHTMAP && idxLightmapStage == -1)
			{
				idxLightmapStage = j + i;
				tmpLightmapStage = stages[j + i];
			}
		}

		// try to merge color/lightmap to diffuse
		if (idxColorStage != -1 &&
		    idxLightmapStage != -1 &&
		    // TODO check color stage no alphaGen
		    (tmpLightmapStage.stateBits & (GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO))
		    )
		{
			Ren_Print("color/lightmap combo\n");

			tmpShader.collapseType = COLLAPSE_color_lightmap;

			tmpStages[numStages]            = tmpColorStage;
			tmpStages[numStages].type       = ST_DIFFUSEMAP;
			tmpStages[numStages].stateBits &= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
			//tmpStages[numStages].stateBits |= GLS_DEPTHMASK_TRUE;

			//tmpStages[numStages].bundle[TB_NORMALMAP] = tmpNormalStage.bundle[0];

			numStages++;
			j += 1;
			continue;
		}
		/*
		else if(idxLightmapStage > idxColorStage)
		{
		    tmpStages[numStages] = tmpColorStage;
		    numStages++;

		    tmpStages[numStages] = tmpLightmapStage;
		    numStages++;
		    continue;
		}
		else
		{
		    tmpStages[numStages] = tmpLightmapStage;
		    numStages++;

		    tmpStages[numStages] = tmpColorStage;
		    numStages++;
		    continue;
		}
		*/
#endif

		for (i = 0; i < 3; i++)
		{
			if ((j + i) >= MAX_SHADER_STAGES)
			{
				continue;
			}

			if (!stages[j + i].active)
			{
				continue;
			}

			if (stages[j + i].type == ST_DIFFUSEMAP && !hasDiffuseStage)
			{
				hasDiffuseStage = qtrue;
				tmpDiffuseStage = stages[j + i];
			}
			else if (stages[j + i].type == ST_NORMALMAP && !hasNormalStage)
			{
				hasNormalStage = qtrue;
				tmpNormalStage = stages[j + i];
			}
			else if (stages[j + i].type == ST_SPECULARMAP && !hasSpecularStage)
			{
				hasSpecularStage = qtrue;
				tmpSpecularStage = stages[j + i];
			}
			else if (stages[j + i].type == ST_REFLECTIONMAP && !hasReflectionStage)
			{
				hasReflectionStage = qtrue;
				tmpReflectionStage = stages[j + i];
			}
		}

		// NOTE: merge as many stages as possible

		// try to merge diffuse/normal/specular
		if (hasDiffuseStage     &&
		    hasNormalStage      &&
		    hasSpecularStage
		    )
		{
			//Ren_Print("lighting_DBS\n");

			tmpShader.collapseType = COLLAPSE_lighting_DBS;

			tmpStages[numStages]      = tmpDiffuseStage;
			tmpStages[numStages].type = ST_COLLAPSE_lighting_DBS;

			tmpStages[numStages].bundle[TB_NORMALMAP]   = tmpNormalStage.bundle[0];
			tmpStages[numStages].bundle[TB_SPECULARMAP] = tmpSpecularStage.bundle[0];

			numStages++;
			j += 2;
			continue;
		}
		// try to merge diffuse/normal
		else if (hasDiffuseStage     &&
		         hasNormalStage
		         )
		{
			//Ren_Print("lighting_DB\n");

			tmpShader.collapseType = COLLAPSE_lighting_DB;

			tmpStages[numStages]      = tmpDiffuseStage;
			tmpStages[numStages].type = ST_COLLAPSE_lighting_DB;

			tmpStages[numStages].bundle[TB_NORMALMAP] = tmpNormalStage.bundle[0];

			numStages++;
			j += 1;
			continue;
		}
		// try to merge env/normal
		else if (hasReflectionStage &&
		         hasNormalStage
		         )
		{
			//Ren_Print("reflection_CB\n");

			tmpShader.collapseType = COLLAPSE_reflection_CB;

			tmpStages[numStages]      = tmpReflectionStage;
			tmpStages[numStages].type = ST_COLLAPSE_reflection_CB;

			tmpStages[numStages].bundle[TB_NORMALMAP] = tmpNormalStage.bundle[0];

			numStages++;
			j += 1;
			continue;
		}
		// if there was no merge option just copy stage
		else
		{
			tmpStages[numStages] = stages[j];
			numStages++;
		}
	}

	// clear unused stages
	Com_Memset(&tmpStages[numStages], 0, sizeof(stages[0]) * (MAX_SHADER_STAGES - numStages));
	tmpShader.numStages = numStages;

	// copy result
	Com_Memcpy(&stages[0], &tmpStages[0], sizeof(stages));
	Com_Memcpy(&shader, &tmpShader, sizeof(shader));
}
// *INDENT-ON*

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
static void FixRenderCommandList(int newShader)
{
	renderCommandList_t *cmdList = &backEndData->commands;

	if (cmdList)
	{
		const void *curCmd = cmdList->cmds;

		while (1)
		{
			switch (*(const int *)curCmd)
			{
			case RC_SET_COLOR:
			{
				const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;

				curCmd = (const void *)(sc_cmd + 1);
				break;
			}
			case RC_STRETCH_PIC:
			{
				const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;

				curCmd = (const void *)(sp_cmd + 1);
				break;
			}
			case RC_DRAW_VIEW:
			{
				int                     i;
				drawSurf_t              *drawSurf;
				const drawViewCommand_t *dv_cmd = (const drawViewCommand_t *)curCmd;

				for (i = 0, drawSurf = dv_cmd->viewParms.drawSurfs; i < dv_cmd->viewParms.numDrawSurfs; i++, drawSurf++)
				{
					if (drawSurf->shaderNum >= newShader)
					{
						drawSurf->shaderNum++;
					}
				}
				curCmd = (const void *)(dv_cmd + 1);
				break;
			}
			case RC_DRAW_BUFFER:
			{
				const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;

				curCmd = (const void *)(db_cmd + 1);
				break;
			}
			case RC_SWAP_BUFFERS:
			{
				const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;

				curCmd = (const void *)(sb_cmd + 1);
				break;
			}
			case RC_END_OF_LIST:
			default:
				return;
			}
		}
	}
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader(void)
{
	int      i;
	float    sort;
	shader_t *newShader;

	newShader = tr.shaders[tr.numShaders - 1];
	sort      = newShader->sort;

	for (i = tr.numShaders - 2; i >= 0; i--)
	{
		if (tr.sortedShaders[i]->sort <= sort)
		{
			break;
		}
		tr.sortedShaders[i + 1] = tr.sortedShaders[i];
		tr.sortedShaders[i + 1]->sortedIndex++;
	}

	// fix rendercommandlist
	FixRenderCommandList(i + 1);

	newShader->sortedIndex  = i + 1;
	tr.sortedShaders[i + 1] = newShader;
}

/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader(void)
{
	shader_t *newShader;
	int      i, b;
	int      size, hash;

	if (tr.numShaders == MAX_SHADERS)
	{
		Ren_Warning("WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = (shader_t *)ri.Hunk_Alloc(sizeof(shader_t), h_low);

	*newShader = shader;

	if (shader.sort <= SS_OPAQUE)
	{
		newShader->fogPass = FP_EQUAL;
	}
	else if (shader.contentFlags & CONTENTS_FOG)
	{
		newShader->fogPass = FP_LE;
	}

	tr.shaders[tr.numShaders] = newShader;
	newShader->index          = tr.numShaders;

	tr.sortedShaders[tr.numShaders] = newShader;
	newShader->sortedIndex          = tr.numShaders;

	tr.numShaders++;

	for (i = 0; i < newShader->numStages; i++)
	{
		if (!stages[i].active)
		{
			break;
		}
		newShader->stages[i]  = (shaderStage_t *)ri.Hunk_Alloc(sizeof(stages[i]), h_low);
		*newShader->stages[i] = stages[i];

		for (b = 0; b < MAX_TEXTURE_BUNDLES; b++)
		{
			size                                    = newShader->stages[i]->bundle[b].numTexMods * sizeof(texModInfo_t);
			newShader->stages[i]->bundle[b].texMods = (texModInfo_t *)ri.Hunk_Alloc(size, h_low);
			Com_Memcpy(newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size);
		}
	}

	SortNewShader();

	hash                  = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next       = shaderHashTable[hash];
	shaderHashTable[hash] = newShader;

	return newShader;
}

/*
====================
GeneratePermanentShaderTable
====================
*/
static void GeneratePermanentShaderTable(float *values, int numValues)
{
	shaderTable_t *newTable;
	int           i;
	int           hash;

	if (tr.numTables == MAX_SHADER_TABLES)
	{
		Ren_Warning("WARNING: GeneratePermanentShaderTables - MAX_SHADER_TABLES hit\n");
		return;
	}

	newTable = (shaderTable_t *)ri.Hunk_Alloc(sizeof(shaderTable_t), h_low);

	*newTable = table;

	tr.shaderTables[tr.numTables] = newTable;
	newTable->index               = tr.numTables;

	tr.numTables++;

	newTable->numValues = numValues;
	newTable->values    = (float *)ri.Hunk_Alloc(sizeof(float) * numValues, h_low);

	//Ren_Print("values: \n");
	for (i = 0; i < numValues; i++)
	{
		newTable->values[i] = values[i];

		//Ren_Print("%f", newTable->values[i]);

		//if(i != numValues -1)
		//  Ren_Print(", ");
	}
	//Ren_Print("\n");

	hash                       = generateHashValue(newTable->name, MAX_SHADERTABLE_HASH);
	newTable->next             = shaderTableHashTable[hash];
	shaderTableHashTable[hash] = newTable;
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader(void)
{
	int stage, i;

	// set sky stuff appropriate
	if (shader.isSky)
	{
		if (shader.noFog)
		{
			shader.sort = SS_ENVIRONMENT_NOFOG;
		}
		else
		{
			shader.sort = SS_ENVIRONMENT_FOG;
		}
	}

	if (shader.forceOpaque)
	{
		shader.sort = SS_OPAQUE;
	}

	// set polygon offset
	if (shader.polygonOffset && !shader.sort)
	{
		shader.sort = SS_DECAL;
	}

	// all light materials need at least one z attenuation stage as first stage
	if (shader.type == SHADER_LIGHT)
	{
		if (stages[0].type != ST_ATTENUATIONMAP_Z)
		{
			// move up subsequent stages
			memmove(&stages[1], &stages[0], sizeof(stages[0]) * (MAX_SHADER_STAGES - 1));

			stages[0].active           = qtrue;
			stages[0].type             = ST_ATTENUATIONMAP_Z;
			stages[0].rgbGen           = CGEN_IDENTITY;
			stages[0].stateBits        = GLS_DEFAULT;
			stages[0].overrideWrapType = qtrue;
			stages[0].wrapType         = WT_EDGE_CLAMP;

			LoadMap(&stages[0], "lights/squarelight1a.tga");
		}

		// force following shader stages to be xy attenuation stages
		for (stage = 1; stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = &stages[stage];

			if (!pStage->active)
			{
				break;
			}

			pStage->type = ST_ATTENUATIONMAP_XY;
		}
	}

	// set appropriate stage information
	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t *pStage = &stages[stage];

		if (!pStage->active)
		{
			break;
		}

		// check for a missing texture
		switch (pStage->type)
		{
		case ST_LIQUIDMAP:
		case ST_LIGHTMAP:
			// skip
			break;
		case ST_COLORMAP:
		default:
		{
			if (!pStage->bundle[0].image[0])
			{
				Ren_Warning("Shader %s has a colormap stage with no image\n", shader.name);
				pStage->active = qfalse;
				continue;
			}
			break;
		}
		case ST_DIFFUSEMAP:
		{
			if (!shader.isSky)
			{
				shader.interactLight = qtrue;
			}

			if (!pStage->bundle[0].image[0])
			{
				Ren_Warning("Shader %s has a diffusemap stage with no image\n", shader.name);
				pStage->bundle[0].image[0] = tr.defaultImage;
			}
			break;
		}
		case ST_NORMALMAP:
		{
			if (!shader.isSky)
			{
				shader.interactLight = qtrue;
			}

			if (!pStage->bundle[0].image[0])
			{
				Ren_Warning("Shader %s has a normalmap stage with no image\n", shader.name);
				pStage->bundle[0].image[0] = tr.flatImage;
			}
			break;
		}
		case ST_SPECULARMAP:
		{
			if (!pStage->bundle[0].image[0])
			{
				Ren_Warning("Shader %s has a specularmap stage with no image\n", shader.name);
				pStage->bundle[0].image[0] = tr.blackImage;
			}
			break;
		}
		case ST_ATTENUATIONMAP_XY:
		{
			if (!pStage->bundle[0].image[0])
			{
				Ren_Warning("Shader %s has a xy attenuationmap stage with no image\n", shader.name);
				pStage->active = qfalse;
				continue;
			}
			break;
		}
		case ST_ATTENUATIONMAP_Z:
		{
			if (!pStage->bundle[0].image[0])
			{
				Ren_Warning("Shader %s has a z attenuationmap stage with no image\n", shader.name);
				pStage->active = qfalse;
				continue;
			}
			break;
		}
		}

		// ditch this stage if it's detail and detail textures are disabled

		if (pStage->isDetail && !r_detailTextures->integer)
		{
			if (stage < (MAX_SHADER_STAGES - 1))
			{
				memmove(pStage, pStage + 1, sizeof(*pStage) * (MAX_SHADER_STAGES - stage - 1));
				// kill the last stage, since it's now a duplicate
				for (i = MAX_SHADER_STAGES - 1; i > stage; i--)
				{
					if (stages[i].active)
					{
						memset(&stages[i], 0, sizeof(*pStage));
						break;
					}
				}
				stage--;        // the next stage is now the current stage, so check it again
			}
			else
			{
				memset(pStage, 0, sizeof(*pStage));
			}
			continue;
		}

		if (shader.forceOpaque)
		{
			pStage->stateBits |= GLS_DEPTHMASK_TRUE;
		}

		if (shader.isSky && pStage->noFog)
		{
			shader.sort = SS_ENVIRONMENT_NOFOG;
		}

		// determine sort order and fog color adjustment
		if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) &&
		    (stages[0].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)))
		{
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if (((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE)) ||
			    ((blendSrcBits == GLS_SRCBLEND_ZERO) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR)))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ((blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) &&
			         (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			}
			else
			{
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if (!shader.sort)
			{
				// see through item, like a grill or grate
				if (pStage->stateBits & GLS_DEPTHMASK_TRUE)
				{
					shader.sort = SS_SEE_THROUGH;
				}
				else
				{
					shader.sort = SS_BLEND0;
				}
			}
		}
	}
	shader.numStages = stage;

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if (!shader.sort)
	{
		if (shader.translucent && !shader.forceOpaque)
		{
			shader.sort = SS_DECAL;
		}
		else
		{
			shader.sort = SS_OPAQUE;
		}
	}


	// HACK: allow alpha tested surfaces to create shadowmaps
	if (r_shadows->integer >= SHADOWING_ESM16)
	{
		if (shader.noShadows && shader.alphaTest)
		{
			shader.noShadows = qfalse;
		}
	}

	// look for multitexture potential
	CollapseStages();

	// fogonly shaders don't have any normal passes
	if (shader.numStages == 0 && !shader.isSky)
	{
		shader.sort = SS_FOG;
	}

	return GeneratePermanentShader();
}

//========================================================================================

// dynamic shader list
typedef struct dynamicshader dynamicshader_t;
struct dynamicshader
{
	char *shadertext;
	dynamicshader_t *next;
};
static dynamicshader_t *dshader = NULL;

/*
====================
RE_LoadDynamicShader

load a new dynamic shader

if shadertext is NULL, looks for matching shadername and removes it

returns qtrue if request was successful, qfalse if the gods were angered
====================
*/
qboolean RE_LoadDynamicShader(const char *shadername, const char *shadertext)
{
	const char      *func_err = "WARNING: RE_LoadDynamicShader";
	dynamicshader_t *dptr, *lastdptr;
	char            *q, *token;

	Ren_Warning("RE_LoadDynamicShader( name = '%s', text = '%s' )\n", shadername, shadertext);

	if (!shadername && shadertext)
	{
		Ren_Warning("%s called with NULL shadername and non-NULL shadertext:\n%s\n", func_err, shadertext);
		return qfalse;
	}

	if (shadername && strlen(shadername) >= MAX_QPATH)
	{
		Ren_Warning("%s shadername %s exceeds MAX_QPATH\n", func_err, shadername);
		return qfalse;
	}

	// empty the whole list
	if (!shadername && !shadertext)
	{
		dptr = dshader;
		while (dptr)
		{
			lastdptr = dptr->next;
			ri.Free(dptr->shadertext);
			ri.Free(dptr);
			dptr = lastdptr;
		}
		dshader = NULL;
		return qtrue;
	}

	// walk list for existing shader to delete, or end of the list
	dptr     = dshader;
	lastdptr = NULL;
	while (dptr)
	{
		q = dptr->shadertext;

		token = COM_ParseExt(&q, qtrue);

		if ((token[0] != 0) && !Q_stricmp(token, shadername))
		{
			//request to nuke this dynamic shader
			if (!shadertext)
			{
				if (!lastdptr)
				{
					dshader = NULL;
				}
				else
				{
					lastdptr->next = dptr->next;
				}
				ri.Free(dptr->shadertext);
				ri.Free(dptr);
				return qtrue;
			}
			Ren_Warning("%s shader %s already exists!\n", func_err, shadername);
			return qfalse;
		}
		lastdptr = dptr;
		dptr     = dptr->next;
	}

	// cant add a new one with empty shadertext
	if (!shadertext || !strlen(shadertext))
	{
		Ren_Warning("%s new shader %s has NULL shadertext!\n", func_err, shadername);
		return qfalse;
	}

	// create a new shader
	dptr = (dynamicshader_t *) ri.Z_Malloc(sizeof(*dptr));
	if (!dptr)
	{
		Ren_Fatal("Couldn't allocate struct for dynamic shader %s\n", shadername);
	}
	if (lastdptr)
	{
		lastdptr->next = dptr;
	}
	dptr->shadertext = (char *)ri.Z_Malloc(strlen(shadertext) + 1);
	if (!dptr->shadertext)
	{
		Ren_Fatal("Couldn't allocate buffer for dynamic shader %s\n", shadername);
	}
	Q_strncpyz(dptr->shadertext, shadertext, strlen(shadertext) + 1);
	dptr->next = NULL;
	if (!dshader)
	{
		dshader = dptr;
	}

	//ri.Printf( PRINT_ALL, "Loaded dynamic shader [%s] with shadertext [%s]\n", shadername, shadertext );

	return qtrue;
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static char *FindShaderInShaderText(const char *shaderName)
{
	char *token, *p;
	int  i, hash;

	hash = generateHashValue(shaderName, MAX_SHADERTEXT_HASH);

	for (i = 0; shaderTextHashTable[hash][i]; i++)
	{
		p     = shaderTextHashTable[hash][i];
		token = COM_ParseExt2(&p, qtrue);
		if (!Q_stricmp(token, shaderName))
		{
			//Ren_Print("found shader '%s' by hashing\n", shaderName);
			return p;
		}
	}

	p = s_shaderText;

	if (!p)
	{
		return NULL;
	}

	// look for label
	while (1)
	{
		token = COM_ParseExt2(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		if (!Q_stricmp(token, shaderName))
		{
			//Ren_Print("found shader '%s' by linear search\n", shaderName);
			return p;
		}
		// skip shader tables
		else if (!Q_stricmp(token, "table"))
		{
			// skip table name
			token = COM_ParseExt2(&p, qtrue);

			SkipBracedSection(&p);
		}
		// support shader templates
		else if (!Q_stricmp(token, "guide"))
		{
			// parse shader name
			token = COM_ParseExt2(&p, qtrue);

			if (!Q_stricmp(token, shaderName))
			{
				Ren_Print("found shader '%s' by linear search\n", shaderName);
				return p;
			}

			// skip guide name
			token = COM_ParseExt2(&p, qtrue);

			// skip parameters
			token = COM_ParseExt2(&p, qtrue);
			if (Q_stricmp(token, "("))
			{
				break;
			}

			while (1)
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!token[0])
				{
					break;
				}

				if (!Q_stricmp(token, ")"))
				{
					break;
				}
			}

			if (Q_stricmp(token, ")"))
			{
				break;
			}
		}
		else
		{
			// skip the shader body
			SkipBracedSection(&p);
		}
	}

	return NULL;
}

/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName(const char *name)
{
	char     strippedName[MAX_QPATH];
	int      hash;
	shader_t *sh;

	if ((name == NULL) || (name[0] == 0))
	{                           // bk001205
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	// see if the shader is already loaded
	for (sh = shaderHashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with type == SHADER_3D_DYNAMIC, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0)
		{
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If type == SHADER_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If type == SHADER_3D_DYNAMIC, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If type == SHADER_3D_STATIC, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.
===============
*/
shader_t *R_FindShader(const char *name, shaderType_t type, qboolean mipRawImage)
{
	char     strippedName[MAX_QPATH];
	char     fileName[MAX_QPATH];
	int      i, hash;
	char     *shaderText;
	image_t  *image;
	shader_t *sh;

	if (name[0] == 0)
	{
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	// see if the shader is already loaded
	for (sh = shaderHashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with type == SHADER_3D_DYNAMIC, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ((sh->type == type || sh->defaultShader) && !Q_stricmp(sh->name, strippedName))
		{
			// match found
			return sh;
		}
	}

	// clear the global shader
	Com_Memset(&shader, 0, sizeof(shader));
	Com_Memset(&stages, 0, sizeof(stages));
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	shader.type = type;
	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		stages[i].bundle[0].texMods = texMods[i];
	}

	// default to no implicit mappings
	implicitMap[0]    = '\0';
	implicitStateBits = GLS_DEFAULT;
	implicitCullType  = CT_FRONT_SIDED;

	// attempt to define shader from an explicit parameter file
	shaderText = FindShaderInShaderText(strippedName);
	if (shaderText)
	{
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if (r_printShaders->integer)
		{
			Ren_Print("...loading explicit shader '%s'\n", strippedName);
		}

		if (!ParseShader(shaderText))
		{
			// had errors, so use default shader
			shader.defaultShader = qtrue;
			sh                   = FinishShader();
			return sh;
		}

		// allow implicit mappings
		if (implicitMap[0] == '\0')
		{
			sh = FinishShader();
			return sh;
		}
	}

	// allow implicit mapping ('-' = use shader name)
	if (implicitMap[0] == '\0' || implicitMap[0] == '-')
	{
		Q_strncpyz(fileName, strippedName, sizeof(fileName));
	}
	else
	{
		Q_strncpyz(fileName, implicitMap, sizeof(fileName));
	}

	// implicit shaders were breaking nopicmip/nomipmaps
	if (!mipRawImage)
	{
		//shader.noMipMaps = qtrue;
		shader.noPicMip = qtrue;
	}

	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file
	image = R_FindImageFile(fileName, mipRawImage ? IF_NONE : IF_NOPICMIP,
	                        mipRawImage ? FT_DEFAULT : FT_LINEAR, mipRawImage ? WT_REPEAT : WT_CLAMP, shader.name);
	if (!image)
	{
		Ren_Developer("Couldn't find image file for shader %s\n", name);
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	// set implicit cull type
	if (implicitCullType && !shader.cullType)
	{
		shader.cullType = implicitCullType;
	}

	// create the default shading commands
	switch (shader.type)
	{
	case SHADER_2D:
	{
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active             = qtrue;
		stages[0].rgbGen             = CGEN_VERTEX;
		stages[0].alphaGen           = AGEN_VERTEX;
		stages[0].stateBits          = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		break;
	}
	case SHADER_3D_DYNAMIC:
	{
		// dynamic colors at vertexes
		stages[0].type               = ST_DIFFUSEMAP;
		stages[0].bundle[0].image[0] = image;
		stages[0].active             = qtrue;
		stages[0].rgbGen             = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits          = implicitStateBits;
		break;
	}
	case SHADER_3D_STATIC:
	{
		// explicit colors at vertexes
		stages[0].type               = ST_DIFFUSEMAP;
		stages[0].bundle[0].image[0] = image;
		stages[0].active             = qtrue;
		stages[0].rgbGen             = CGEN_VERTEX;
		stages[0].stateBits          = implicitStateBits;
		break;
	}
	case SHADER_LIGHT:
	{
		stages[0].type               = ST_ATTENUATIONMAP_Z;
		stages[0].bundle[0].image[0] = tr.noFalloffImage;       // FIXME should be attenuationZImage
		stages[0].active             = qtrue;
		stages[0].rgbGen             = CGEN_IDENTITY;
		stages[0].stateBits          = GLS_DEFAULT;

		stages[1].type               = ST_ATTENUATIONMAP_XY;
		stages[1].bundle[0].image[0] = image;
		stages[1].active             = qtrue;
		stages[1].rgbGen             = CGEN_IDENTITY;
		stages[1].stateBits          = GLS_DEFAULT;
		//stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		break;
	}
	default:
		break;
	}

	return FinishShader();
}

qhandle_t RE_RegisterShaderFromImage(const char *name, image_t *image, qboolean mipRawImage)
{
	int      i, hash;
	shader_t *sh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// see if the shader is already loaded
	for (sh = shaderHashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with type == SHADER_3D_DYNAMIC, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ((sh->type == SHADER_2D || sh->defaultShader) && !Q_stricmp(sh->name, name))
		{
			// match found
			return sh->index;
		}
	}

	// clear the global shader
	Com_Memset(&shader, 0, sizeof(shader));
	Com_Memset(&stages, 0, sizeof(stages));
	Q_strncpyz(shader.name, name, sizeof(shader.name));
	shader.type = SHADER_2D;
	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		stages[i].bundle[0].texMods = texMods[i];
	}

	// create the default shading commands

	// GUI elements
	stages[0].bundle[0].image[0] = image;
	stages[0].active             = qtrue;
	stages[0].rgbGen             = CGEN_VERTEX;
	stages[0].alphaGen           = AGEN_VERTEX;
	stages[0].stateBits          = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;

	sh = FinishShader();
	return sh->index;
}

/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader(const char *name)
{
	shader_t *sh;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Print("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, SHADER_2D, qtrue);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		return 0;
	}

	return sh->index;
}

/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip(const char *name)
{
	shader_t *sh;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Print("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, SHADER_2D, qfalse);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again

	if (sh->defaultShader)
	{
		return 0;
	}

	return sh->index;
}

/*
====================
RE_RegisterShaderLightAttenuation

For different Doom3 style light effects
====================
*/
qhandle_t RE_RegisterShaderLightAttenuation(const char *name)
{
	shader_t *sh;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Print("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, SHADER_LIGHT, qfalse);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		return 0;
	}

	return sh->index;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle(qhandle_t hShader)
{
	if (hShader < 0)
	{
		Ren_Warning("R_GetShaderByHandle: out of range hShader '%d'\n", hShader);   // bk: FIXME name
		return tr.defaultShader;
	}
	if (hShader >= tr.numShaders)
	{
		Ren_Warning("R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void R_ShaderList_f(void)
{
	int      i;
	int      count;
	shader_t *shader;
	char     *s = NULL;

	Ren_Print("-----------------------\n");

	if (ri.Cmd_Argc() > 1)
	{
		s = ri.Cmd_Argv(1);
	}

	count = 0;
	for (i = 0; i < tr.numShaders; i++)
	{
		if (ri.Cmd_Argc() > 2)
		{
			shader = tr.sortedShaders[i];
		}
		else
		{
			shader = tr.shaders[i];
		}

		if (s && Q_stricmpn(shader->name, s, strlen(s)) != 0)
		{
			continue;
		}

		Ren_Print("%i ", shader->numStages);

		switch (shader->type)
		{
		case SHADER_2D:
			Ren_Print("2D   ");
			break;
		case SHADER_3D_DYNAMIC:
			Ren_Print("3D_D ");
			break;
		case SHADER_3D_STATIC:
			Ren_Print("3D_S ");
			break;
		case SHADER_LIGHT:
			Ren_Print("ATTN ");
			break;
		}

		/*
		if(shader->collapseType == COLLAPSE_genericMulti)
		{
		    if(shader->collapseTextureEnv == GL_ADD)
		    {
		        Ren_Print("MT(a)          ");
		    }
		    else if(shader->collapseTextureEnv == GL_MODULATE)
		    {
		        Ren_Print("MT(m)          ");
		    }
		    else if(shader->collapseTextureEnv == GL_DECAL)
		    {
		        Ren_Print("MT(d)          ");
		    }
		}
		else */
		if (shader->collapseType == COLLAPSE_lighting_DB)
		{
			Ren_Print("lighting_DB    ");
		}
		else if (shader->collapseType == COLLAPSE_lighting_DBS)
		{
			Ren_Print("lighting_DBS   ");
		}
		else if (shader->collapseType == COLLAPSE_reflection_CB)
		{
			Ren_Print("reflection_CB  ");
		}
		else
		{
			Ren_Print("               ");
		}

		if (shader->createdByGuide)
		{
			Ren_Print("G ");
		}
		else if (shader->explicitlyDefined)
		{
			Ren_Print("E ");
		}
		else
		{
			Ren_Print("  ");
		}

		if (shader->sort == SS_BAD)
		{
			Ren_Print("SS_BAD              ");
		}
		else if (shader->sort == SS_PORTAL)
		{
			Ren_Print("SS_PORTAL           ");
		}
		else if (shader->sort == SS_ENVIRONMENT_FOG)
		{
			Ren_Print("SS_ENVIRONMENT_FOG  ");
		}
		else if (shader->sort == SS_ENVIRONMENT_NOFOG)
		{
			Ren_Print("SS_ENVIRONMENT_NOFOG");
		}
		else if (shader->sort == SS_OPAQUE)
		{
			Ren_Print("SS_OPAQUE           ");
		}
		else if (shader->sort == SS_DECAL)
		{
			Ren_Print("SS_DECAL            ");
		}
		else if (shader->sort == SS_SEE_THROUGH)
		{
			Ren_Print("SS_SEE_THROUGH      ");
		}
		else if (shader->sort == SS_BANNER)
		{
			Ren_Print("SS_BANNER           ");
		}
		else if (shader->sort == SS_FOG)
		{
			Ren_Print("SS_FOG              ");
		}
		else if (shader->sort == SS_UNDERWATER)
		{
			Ren_Print("SS_UNDERWATER       ");
		}
		else if (shader->sort == SS_WATER)
		{
			Ren_Print("SS_WATER            ");
		}
		else if (shader->sort == SS_FAR)
		{
			Ren_Print("SS_FAR              ");
		}
		else if (shader->sort == SS_MEDIUM)
		{
			Ren_Print("SS_MEDIUM           ");
		}
		else if (shader->sort == SS_CLOSE)
		{
			Ren_Print("SS_CLOSE            ");
		}
		else if (shader->sort == SS_BLEND0)
		{
			Ren_Print("SS_BLEND0           ");
		}
		else if (shader->sort == SS_BLEND1)
		{
			Ren_Print("SS_BLEND1           ");
		}
		else if (shader->sort == SS_BLEND2)
		{
			Ren_Print("SS_BLEND2           ");
		}
		else if (shader->sort == SS_BLEND3)
		{
			Ren_Print("SS_BLEND3           ");
		}
		else if (shader->sort == SS_BLEND6)
		{
			Ren_Print("SS_BLEND6           ");
		}
		else if (shader->sort == SS_ALMOST_NEAREST)
		{
			Ren_Print("SS_ALMOST_NEAREST   ");
		}
		else if (shader->sort == SS_NEAREST)
		{
			Ren_Print("SS_NEAREST          ");
		}
		else if (shader->sort == SS_POST_PROCESS)
		{
			Ren_Print("SS_POST_PROCESS     ");
		}
		else
		{
			Ren_Print("                    ");
		}

		if (shader->interactLight)
		{
			Ren_Print("IA ");
		}
		else
		{
			Ren_Print("   ");
		}

		if (shader->defaultShader)
		{
			Ren_Print(": %s (DEFAULTED)\n", shader->name);
		}
		else
		{
			Ren_Print(": %s\n", shader->name);
		}
		count++;
	}
	Ren_Print("%i total shaders\n", count);
	Ren_Print("------------------\n");
}

void R_ShaderExp_f(void)
{
	int          i;
	int          len;
	char         buffer[1024] = "";
	char         *buffer_p    = &buffer[0];
	expression_t exp;

	strcpy(shader.name, "dummy");

	Ren_Print("-----------------------\n");

	for (i = 1; i < ri.Cmd_Argc(); i++)
	{
		strcat(buffer, ri.Cmd_Argv(i));
		strcat(buffer, " ");
	}
	len             = strlen(buffer);
	buffer[len - 1] = 0;        // replace last " " with tailing zero

	ParseExpression(&buffer_p, &exp);

	Ren_Print("%i total ops\n", exp.numOps);
	Ren_Print("%f result\n", RB_EvalExpression(&exp, 0));
	Ren_Print("------------------\n");
}

/*
====================
ScanAndLoadShaderGuides

Finds and loads all .guide files, combining them into
a single large text block that can be scanned for shader template names
=====================
*/
#define MAX_GUIDE_FILES 1024
static void ScanAndLoadGuideFiles(void)
{
	char **guideFiles;
	char *buffers[MAX_GUIDE_FILES];
	char *p;
	int  numGuides;
	int  i;
	char *oldp, *token, *hashMem;
	int  guideTextHashTableSizes[MAX_GUIDETEXT_HASH], hash, size;
	char filename[MAX_QPATH];
	long sum = 0;

	Ren_Print("----- ScanAndLoadGuideFiles -----\n");

	s_guideText = NULL;
	Com_Memset(guideTextHashTableSizes, 0, sizeof(guideTextHashTableSizes));
	Com_Memset(guideTextHashTable, 0, sizeof(guideTextHashTable));

	// scan for guide files
	guideFiles = ri.FS_ListFiles("guides", ".guide", &numGuides);

	if (!guideFiles || !numGuides)
	{
		Ren_Warning("WARNING: no shader guide files found\n");
		return;
	}

	if (numGuides > MAX_GUIDE_FILES)
	{
		numGuides = MAX_GUIDE_FILES;
	}

	// build single large buffer
	for (i = 0; i < numGuides; i++)
	{
		Com_sprintf(filename, sizeof(filename), "guides/%s", guideFiles[i]);

		sum += ri.FS_ReadFile(filename, NULL);
	}
	s_guideText = (char *)ri.Hunk_Alloc(sum + numGuides * 2, h_low);

	// load in reverse order, so doubled templates are overriden properly
	for (i = numGuides - 1; i >= 0; i--)
	{
		Com_sprintf(filename, sizeof(filename), "guides/%s", guideFiles[i]);

		Ren_Developer("...loading '%s'\n", filename);
		sum += ri.FS_ReadFile(filename, (void **)&buffers[i]);
		if (!buffers[i])
		{
			Ren_Drop("Couldn't load %s", filename);
		}

		strcat(s_guideText, "\n");
		p = &s_guideText[strlen(s_guideText)];
		strcat(s_guideText, buffers[i]);
		ri.FS_FreeFile(buffers[i]);
		buffers[i] = p;
		COM_Compress(p);
	}

	size = 0;
	for (i = 0; i < numGuides; i++)
	{
		Com_sprintf(filename, sizeof(filename), "guides/%s", guideFiles[i]);

		COM_BeginParseSession(filename);

		// pointer to the first shader file
		p = buffers[i];

		// look for label
		while (1)
		{
			token = COM_ParseExt2(&p, qtrue);
			if (token[0] == 0)
			{
				break;
			}

			if (Q_stricmp(token, "guide") && Q_stricmp(token, "inlineGuide"))
			{
				COM_ParseWarning("expected guide or inlineGuide found '%s'\n", token);
				break;
			}

			// parse guide name
			token = COM_ParseExt2(&p, qtrue);

			//Ren_Print("guide: '%s'\n", token);

			hash = generateHashValue(token, MAX_GUIDETEXT_HASH);
			guideTextHashTableSizes[hash]++;
			size++;

			// skip parameters
			token = COM_ParseExt2(&p, qtrue);
			if (Q_stricmp(token, "("))
			{
				COM_ParseWarning("expected ( found '%s'\n", token);
				break;
			}

			while (1)
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!token[0])
				{
					break;
				}

				if (!Q_stricmp(token, ")"))
				{
					break;
				}
			}

			if (Q_stricmp(token, ")"))
			{
				COM_ParseWarning("expected ) found '%s'\n", token);
				break;
			}

			// skip guide body
			SkipBracedSection(&p);

			// if we passed the pointer to the next shader file
			if (i < numGuides - 1)
			{
				if (p > buffers[i + 1])
				{
					break;
				}
			}
		}
	}

	size += MAX_GUIDETEXT_HASH;

	hashMem = (char *)ri.Hunk_Alloc(size * sizeof(char *), h_low);

	for (i = 0; i < MAX_GUIDETEXT_HASH; i++)
	{
		guideTextHashTable[i] = (char **)hashMem;
		hashMem               = ((char *)hashMem) + ((guideTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	Com_Memset(guideTextHashTableSizes, 0, sizeof(guideTextHashTableSizes));

	for (i = 0; i < numGuides; i++)
	{
		Com_sprintf(filename, sizeof(filename), "guides/%s", guideFiles[i]);

		COM_BeginParseSession(filename);

		// pointer to the first shader file
		p = buffers[i];

		// look for label
		while (1)
		{
			token = COM_ParseExt2(&p, qtrue);
			if (token[0] == 0)
			{
				break;
			}

			if (Q_stricmp(token, "guide") && Q_stricmp(token, "inlineGuide"))
			{
				COM_ParseWarning("expected guide or inlineGuide found '%s'\n", token);
				break;
			}

			// parse guide name
			oldp  = p;
			token = COM_ParseExt2(&p, qtrue);

			//Ren_Print("...hashing guide '%s'\n", token);

			hash                                                      = generateHashValue(token, MAX_GUIDETEXT_HASH);
			guideTextHashTable[hash][guideTextHashTableSizes[hash]++] = oldp;

			// skip parameters
			token = COM_ParseExt2(&p, qtrue);
			if (Q_stricmp(token, "("))
			{
				COM_ParseWarning("expected ( found '%s'\n", token);
				break;
			}

			while (1)
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!token[0])
				{
					break;
				}

				if (!Q_stricmp(token, ")"))
				{
					break;
				}
			}

			if (Q_stricmp(token, ")"))
			{
				COM_ParseWarning("expected ) found '%s'\n", token);
				break;
			}

			// skip guide body
			SkipBracedSection(&p);

			// if we passed the pointer to the next shader file
			if (i < numGuides - 1)
			{
				if (p > buffers[i + 1])
				{
					break;
				}
			}
		}
	}

	// free up memory
	ri.FS_FreeFileList(guideFiles);
}

/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define MAX_SHADER_FILES    4096
static void ScanAndLoadShaderFiles(void)
{
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	char *p;
	int  numShaderFiles;
	int  i;
	char *oldp, *token, *hashMem, *textEnd;
	int  shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;
	char filename[MAX_QPATH];
	long sum = 0, summand;

	Ren_Print("----- ScanAndLoadShaderFiles -----\n");

	// scan for shader files
	shaderFiles = ri.FS_ListFiles("scripts", ".shader", &numShaderFiles);

	if (!shaderFiles || !numShaderFiles)
	{
		Ren_Warning("WARNING: ScanAndLoadShaderFiles: no shader files found\n");
		return;
	}

	// build single large buffer
	for (i = 0; i < numShaderFiles; i++)
	{
		Com_sprintf(filename, sizeof(filename), "scripts/%s", shaderFiles[i]);
		sum += ri.FS_ReadFile(filename, NULL);
	}
	s_shaderText = (char *)ri.Hunk_Alloc(sum + numShaderFiles * 2, h_low);

	if (numShaderFiles > MAX_SHADER_FILES)
	{
		numShaderFiles = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for (i = 0; i < numShaderFiles; i++)
	{
		Com_sprintf(filename, sizeof(filename), "scripts/%s", shaderFiles[i]);

		Ren_Developer("...loading '%s'\n", filename);
		summand = ri.FS_ReadFile(filename, (void **)&buffers[i]);

		if (!buffers[i])
		{
			Ren_Drop("Couldn't load %s", filename);
		}

		p = buffers[i];
		while (1)
		{
			token = COM_ParseExt2(&p, qtrue);

			if (!*token)
			{
				break;
			}

			// Step over the "table"/"guide" and the name
			if (!Q_stricmp(token, "table") || !Q_stricmp(token, "guide"))
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!*token)
				{
					break;
				}
			}

			oldp = p;

			token = COM_ParseExt2(&p, qtrue);
			if (token[0] != '{' && token[1] != '\0')
			{
				Ren_Warning("WARNING: Bad shader file %s has incorrect syntax.\n", filename);
				ri.FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			SkipBracedSection(&oldp);
			p = oldp;
		}

		if (buffers[i])
		{
			sum += summand;
		}
	}

	// build single large buffer
	s_shaderText    = (char *)ri.Hunk_Alloc(sum + numShaderFiles * 2, h_low);
	s_shaderText[0] = '\0';
	textEnd         = s_shaderText;

	// free in reverse order, so the temp files are all dumped
	for (i = numShaderFiles - 1; i >= 0 ; i--)
	{
		if (!buffers[i])
		{
			continue;
		}

		strcat(textEnd, buffers[i]);
		strcat(textEnd, "\n");
		textEnd += strlen(textEnd);
		ri.FS_FreeFile(buffers[i]);
	}

	COM_Compress(s_shaderText);

	// free up memory
	ri.FS_FreeFileList(shaderFiles);

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	// look for shader names
	while (1)
	{
		token = COM_ParseExt2(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		// skip shader tables
		if (!Q_stricmp(token, "table"))
		{
			// skip table name
			token = COM_ParseExt2(&p, qtrue);

			SkipBracedSection(&p);
		}
		// support shader templates
		else if (!Q_stricmp(token, "guide"))
		{
			// parse shader name
			token = COM_ParseExt2(&p, qtrue);
			//Ren_Print("...guided '%s'\n", token);

			hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTableSizes[hash]++;
			size++;

			// skip guide name
			token = COM_ParseExt2(&p, qtrue);

			// skip parameters
			token = COM_ParseExt2(&p, qtrue);
			if (Q_stricmp(token, "("))
			{
				Ren_Warning("expected ( found '%s'\n", token);
				//COM_ParseWarning("expected ( found '%s'\n", token);
				break;
			}

			while (1)
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!token[0])
				{
					break;
				}

				if (!Q_stricmp(token, ")"))
				{
					break;
				}
			}

			if (Q_stricmp(token, ")"))
			{
				Ren_Warning("expected ( found '%s'\n", token);
				//COM_ParseWarning("expected ) found '%s'\n", token);
				break;
			}
		}
		else
		{
			hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTableSizes[hash]++;
			size++;
			SkipBracedSection(&p);
		}
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = (char *)ri.Hunk_Alloc(size * sizeof(char *), h_low);

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++)
	{
		shaderTextHashTable[i] = (char **)hashMem;
		hashMem                = ((char *)hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;

	// look for shader names
	while (1)
	{
		oldp  = p;
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		// parse shader tables
		if (!Q_stricmp(token, "table"))
		{
			int           depth;
			float         values[FUNCTABLE_SIZE];
			int           numValues;
			shaderTable_t *tb;
			qboolean      alreadyCreated;

			Com_Memset(&table, 0, sizeof(table));

			token = COM_ParseExt2(&p, qtrue);

			Q_strncpyz(table.name, token, sizeof(table.name));

			// check if already created
			alreadyCreated = qfalse;
			hash           = generateHashValue(table.name, MAX_SHADERTABLE_HASH);
			for (tb = shaderTableHashTable[hash]; tb; tb = tb->next)
			{
				if (Q_stricmp(tb->name, table.name) == 0)
				{
					// match found
					alreadyCreated = qtrue;
					break;
				}
			}

			depth     = 0;
			numValues = 0;
			do
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!Q_stricmp(token, "snap"))
				{
					table.snap = qtrue;
				}
				else if (!Q_stricmp(token, "clamp"))
				{
					table.clamp = qtrue;
				}
				else if (token[0] == '{')
				{
					depth++;
				}
				else if (token[0] == '}')
				{
					depth--;
				}
				else if (token[0] == ',')
				{
					continue;
				}
				else
				{
					if (numValues == FUNCTABLE_SIZE)
					{
						Ren_Warning("WARNING: FUNCTABLE_SIZE hit\n");
						break;
					}
					values[numValues++] = atof(token);
				}
			}
			while (depth && p);

			if (!alreadyCreated)
			{
				Ren_Developer("...generating '%s'\n", table.name);
				GeneratePermanentShaderTable(values, numValues);
			}
		}
		// support shader templates
		else if (!Q_stricmp(token, "guide"))
		{
			// parse shader name
			oldp  = p;
			token = COM_ParseExt2(&p, qtrue);

			//Ren_Print("...guided '%s'\n", token);

			hash                                                        = generateHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

			// skip guide name
			token = COM_ParseExt2(&p, qtrue);

			// skip parameters
			token = COM_ParseExt2(&p, qtrue);
			if (Q_stricmp(token, "("))
			{
				Ren_Warning("expected ( found '%s'\n", token);
				//COM_ParseWarning("expected ( found '%s'\n", token);
				break;
			}

			while (1)
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!token[0])
				{
					break;
				}

				if (!Q_stricmp(token, ")"))
				{
					break;
				}
			}

			if (Q_stricmp(token, ")"))
			{
				Ren_Warning("expected ( found '%s'\n", token);
				//COM_ParseWarning("expected ) found '%s'\n", token);
				break;
			}
		}
		else
		{
			hash                                                        = generateHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

			SkipBracedSection(&p);
		}
	}
}

/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders(void)
{
	Ren_Print("----- CreateInternalShaders -----\n");

	tr.numShaders = 0;

	// init the default shader
	Com_Memset(&shader, 0, sizeof(shader));
	Com_Memset(&stages, 0, sizeof(stages));

	Q_strncpyz(shader.name, "<default>", sizeof(shader.name));

	shader.type                  = SHADER_3D_DYNAMIC;
	stages[0].type               = ST_DIFFUSEMAP;
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active             = qtrue;
	stages[0].stateBits          = GLS_DEFAULT;
	tr.defaultShader             = FinishShader();

	// light shader
	/*
	   Q_strncpyz(shader.name, "<light>", sizeof(shader.name));
	   stages[0].type = ST_ATTENUATIONMAP_Z;
	   stages[0].bundle[0].image[0] = tr.attenuationZImage;
	   stages[0].active = qtrue;
	   stages[0].stateBits = GLS_DEFAULT;

	   stages[1].type = ST_ATTENUATIONMAP_XY;
	   stages[1].bundle[0].image[0] = tr.attenuationXYImage;
	   stages[1].active = qtrue;
	   stages[1].stateBits = GLS_DEFAULT;
	   tr.defaultLightShader = FinishShader();
	 */
}

static void CreateExternalShaders(void)
{
	Ren_Print("----- CreateExternalShaders -----\n");

	tr.flareShader = R_FindShader("flareShader", SHADER_3D_DYNAMIC, qtrue);
	tr.sunShader   = R_FindShader("sun", SHADER_3D_DYNAMIC, qtrue);

	tr.defaultPointLightShader     = R_FindShader("lights/defaultPointLight", SHADER_LIGHT, qtrue);
	tr.defaultProjectedLightShader = R_FindShader("lights/defaultProjectedLight", SHADER_LIGHT, qtrue);
	tr.defaultDynamicLightShader   = R_FindShader("lights/defaultDynamicLight", SHADER_LIGHT, qtrue);
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders(void)
{
	Com_Memset(shaderTableHashTable, 0, sizeof(shaderTableHashTable));
	Com_Memset(shaderHashTable, 0, sizeof(shaderHashTable));

	deferLoad = qfalse;

	CreateInternalShaders();

	ScanAndLoadGuideFiles();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();
}
