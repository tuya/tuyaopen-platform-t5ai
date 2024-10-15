//TODO
#ifndef ZEPHYR_INCLUDE_SYS_UTIL_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_H_
#include <stdint.h>
#ifndef BIT_MASK
#define BIT_MASK(n) (BIT(n) - 1UL)
#endif

#define UTIL_EMPTY(...)
#define UTIL_DEFER(...) __VA_ARGS__ UTIL_EMPTY()
#define UTIL_OBSTRUCT(...) __VA_ARGS__ UTIL_DEFER(UTIL_EMPTY)()
#define UTIL_EXPAND(...) __VA_ARGS__

#define UTIL_EVAL(...)  UTIL_EVAL1(UTIL_EVAL1(UTIL_EVAL1(__VA_ARGS__)))
#define UTIL_EVAL1(...) UTIL_EVAL2(UTIL_EVAL2(UTIL_EVAL2(__VA_ARGS__)))
#define UTIL_EVAL2(...) UTIL_EVAL3(UTIL_EVAL3(UTIL_EVAL3(__VA_ARGS__)))
#define UTIL_EVAL3(...) UTIL_EVAL4(UTIL_EVAL4(UTIL_EVAL4(__VA_ARGS__)))
#define UTIL_EVAL4(...) UTIL_EVAL5(UTIL_EVAL5(UTIL_EVAL5(__VA_ARGS__)))
#define UTIL_EVAL5(...) __VA_ARGS__

#define UTIL_CAT(a, ...) UTIL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define UTIL_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define UTIL_DEC(x) UTIL_PRIMITIVE_CAT(UTIL_DEC_, x)

#define UTIL_DEC_0 0
#define UTIL_DEC_1 0
#define UTIL_DEC_2 1
#define UTIL_DEC_3 2
#define UTIL_DEC_4 3
#define UTIL_DEC_5 4
#define UTIL_DEC_6 5
#define UTIL_DEC_7 6
#define UTIL_DEC_8 7
#define UTIL_DEC_9 8
#define UTIL_DEC_10 9
#define UTIL_DEC_11 10
#define UTIL_DEC_12 11
#define UTIL_DEC_13 12
#define UTIL_DEC_14 13
#define UTIL_DEC_15 14
#define UTIL_DEC_16 15
#define UTIL_DEC_17 16
#define UTIL_DEC_18 17
#define UTIL_DEC_19 18
#define UTIL_DEC_20 19
#define UTIL_DEC_21 20
#define UTIL_DEC_22 21
#define UTIL_DEC_23 22
#define UTIL_DEC_24 23
#define UTIL_DEC_25 24
#define UTIL_DEC_26 25
#define UTIL_DEC_27 26
#define UTIL_DEC_28 27
#define UTIL_DEC_29 28
#define UTIL_DEC_30 29
#define UTIL_DEC_31 30
#define UTIL_DEC_32 31
#define UTIL_DEC_33 32
#define UTIL_DEC_34 33
#define UTIL_DEC_35 34
#define UTIL_DEC_36 35
#define UTIL_DEC_37 36
#define UTIL_DEC_38 37
#define UTIL_DEC_39 38
#define UTIL_DEC_40 39
#define UTIL_DEC_41 40
#define UTIL_DEC_42 41
#define UTIL_DEC_43 42
#define UTIL_DEC_44 43
#define UTIL_DEC_45 44
#define UTIL_DEC_46 45
#define UTIL_DEC_47 46
#define UTIL_DEC_48 47
#define UTIL_DEC_49 48
#define UTIL_DEC_50 49
#define UTIL_DEC_51 50
#define UTIL_DEC_52 51
#define UTIL_DEC_53 52
#define UTIL_DEC_54 53
#define UTIL_DEC_55 54
#define UTIL_DEC_56 55
#define UTIL_DEC_57 56
#define UTIL_DEC_58 57
#define UTIL_DEC_59 58
#define UTIL_DEC_60 59
#define UTIL_DEC_61 60
#define UTIL_DEC_62 61
#define UTIL_DEC_63 62
#define UTIL_DEC_64 63
#define UTIL_DEC_65 64
#define UTIL_DEC_66 65
#define UTIL_DEC_67 66
#define UTIL_DEC_68 67
#define UTIL_DEC_69 68
#define UTIL_DEC_70 69
#define UTIL_DEC_71 70
#define UTIL_DEC_72 71
#define UTIL_DEC_73 72
#define UTIL_DEC_74 73
#define UTIL_DEC_75 74
#define UTIL_DEC_76 75
#define UTIL_DEC_77 76
#define UTIL_DEC_78 77
#define UTIL_DEC_79 78
#define UTIL_DEC_80 79
#define UTIL_DEC_81 80
#define UTIL_DEC_82 81
#define UTIL_DEC_83 82
#define UTIL_DEC_84 83
#define UTIL_DEC_85 84
#define UTIL_DEC_86 85
#define UTIL_DEC_87 86
#define UTIL_DEC_88 87
#define UTIL_DEC_89 88
#define UTIL_DEC_90 89
#define UTIL_DEC_91 90
#define UTIL_DEC_92 91
#define UTIL_DEC_93 92
#define UTIL_DEC_94 93
#define UTIL_DEC_95 94
#define UTIL_DEC_96 95
#define UTIL_DEC_97 96
#define UTIL_DEC_98 97
#define UTIL_DEC_99 98
#define UTIL_DEC_100 99
#define UTIL_DEC_101 100
#define UTIL_DEC_102 101
#define UTIL_DEC_103 102
#define UTIL_DEC_104 103
#define UTIL_DEC_105 104
#define UTIL_DEC_106 105
#define UTIL_DEC_107 106
#define UTIL_DEC_108 107
#define UTIL_DEC_109 108
#define UTIL_DEC_110 109
#define UTIL_DEC_111 110
#define UTIL_DEC_112 111
#define UTIL_DEC_113 112
#define UTIL_DEC_114 113
#define UTIL_DEC_115 114
#define UTIL_DEC_116 115
#define UTIL_DEC_117 116
#define UTIL_DEC_118 117
#define UTIL_DEC_119 118
#define UTIL_DEC_120 119
#define UTIL_DEC_121 120
#define UTIL_DEC_122 121
#define UTIL_DEC_123 122
#define UTIL_DEC_124 123
#define UTIL_DEC_125 124
#define UTIL_DEC_126 125
#define UTIL_DEC_127 126
#define UTIL_DEC_128 127
#define UTIL_DEC_129 128
#define UTIL_DEC_130 129
#define UTIL_DEC_131 130
#define UTIL_DEC_132 131
#define UTIL_DEC_133 132
#define UTIL_DEC_134 133
#define UTIL_DEC_135 134
#define UTIL_DEC_136 135
#define UTIL_DEC_137 136
#define UTIL_DEC_138 137
#define UTIL_DEC_139 138
#define UTIL_DEC_140 139
#define UTIL_DEC_141 140
#define UTIL_DEC_142 141
#define UTIL_DEC_143 142
#define UTIL_DEC_144 143
#define UTIL_DEC_145 144
#define UTIL_DEC_146 145
#define UTIL_DEC_147 146
#define UTIL_DEC_148 147
#define UTIL_DEC_149 148
#define UTIL_DEC_150 149
#define UTIL_DEC_151 150
#define UTIL_DEC_152 151
#define UTIL_DEC_153 152
#define UTIL_DEC_154 153
#define UTIL_DEC_155 154
#define UTIL_DEC_156 155
#define UTIL_DEC_157 156
#define UTIL_DEC_158 157
#define UTIL_DEC_159 158
#define UTIL_DEC_160 159
#define UTIL_DEC_161 160
#define UTIL_DEC_162 161
#define UTIL_DEC_163 162
#define UTIL_DEC_164 163
#define UTIL_DEC_165 164
#define UTIL_DEC_166 165
#define UTIL_DEC_167 166
#define UTIL_DEC_168 167
#define UTIL_DEC_169 168
#define UTIL_DEC_170 169
#define UTIL_DEC_171 170
#define UTIL_DEC_172 171
#define UTIL_DEC_173 172
#define UTIL_DEC_174 173
#define UTIL_DEC_175 174
#define UTIL_DEC_176 175
#define UTIL_DEC_177 176
#define UTIL_DEC_178 177
#define UTIL_DEC_179 178
#define UTIL_DEC_180 179
#define UTIL_DEC_181 180
#define UTIL_DEC_182 181
#define UTIL_DEC_183 182
#define UTIL_DEC_184 183
#define UTIL_DEC_185 184
#define UTIL_DEC_186 185
#define UTIL_DEC_187 186
#define UTIL_DEC_188 187
#define UTIL_DEC_189 188
#define UTIL_DEC_190 189
#define UTIL_DEC_191 190
#define UTIL_DEC_192 191
#define UTIL_DEC_193 192
#define UTIL_DEC_194 193
#define UTIL_DEC_195 194
#define UTIL_DEC_196 195
#define UTIL_DEC_197 196
#define UTIL_DEC_198 197
#define UTIL_DEC_199 198
#define UTIL_DEC_200 199
#define UTIL_DEC_201 200
#define UTIL_DEC_202 201
#define UTIL_DEC_203 202
#define UTIL_DEC_204 203
#define UTIL_DEC_205 204
#define UTIL_DEC_206 205
#define UTIL_DEC_207 206
#define UTIL_DEC_208 207
#define UTIL_DEC_209 208
#define UTIL_DEC_210 209
#define UTIL_DEC_211 210
#define UTIL_DEC_212 211
#define UTIL_DEC_213 212
#define UTIL_DEC_214 213
#define UTIL_DEC_215 214
#define UTIL_DEC_216 215
#define UTIL_DEC_217 216
#define UTIL_DEC_218 217
#define UTIL_DEC_219 218
#define UTIL_DEC_220 219
#define UTIL_DEC_221 220
#define UTIL_DEC_222 221
#define UTIL_DEC_223 222
#define UTIL_DEC_224 223
#define UTIL_DEC_225 224
#define UTIL_DEC_226 225
#define UTIL_DEC_227 226
#define UTIL_DEC_228 227
#define UTIL_DEC_229 228
#define UTIL_DEC_230 229
#define UTIL_DEC_231 230
#define UTIL_DEC_232 231
#define UTIL_DEC_233 232
#define UTIL_DEC_234 233
#define UTIL_DEC_235 234
#define UTIL_DEC_236 235
#define UTIL_DEC_237 236
#define UTIL_DEC_238 237
#define UTIL_DEC_239 238
#define UTIL_DEC_240 239
#define UTIL_DEC_241 240
#define UTIL_DEC_242 241
#define UTIL_DEC_243 242
#define UTIL_DEC_244 243
#define UTIL_DEC_245 244
#define UTIL_DEC_246 245
#define UTIL_DEC_247 246
#define UTIL_DEC_248 247
#define UTIL_DEC_249 248
#define UTIL_DEC_250 249
#define UTIL_DEC_251 250
#define UTIL_DEC_252 251
#define UTIL_DEC_253 252
#define UTIL_DEC_254 253
#define UTIL_DEC_255 254
#define UTIL_DEC_256 255

#define UTIL_CHECK_N(x, n, ...) n
#define UTIL_CHECK(...) UTIL_CHECK_N(__VA_ARGS__, 0,)
#define UTIL_NOT(x) UTIL_CHECK(UTIL_PRIMITIVE_CAT(UTIL_NOT_, x))
#define UTIL_NOT_0 ~, 1,
#define UTIL_COMPL(b) UTIL_PRIMITIVE_CAT(UTIL_COMPL_, b)
#define UTIL_COMPL_0 1
#define UTIL_COMPL_1 0
#define UTIL_BOOL(x) UTIL_COMPL(UTIL_NOT(x))
#define UTIL_IIF(c) UTIL_PRIMITIVE_CAT(UTIL_IIF_, c)
#define UTIL_IIF_0(t, ...) __VA_ARGS__
#define UTIL_IIF_1(t, ...) t
#define UTIL_IF(c) UTIL_IIF(UTIL_BOOL(c))

#define UTIL_EAT(...)
#define UTIL_WHEN(c) UTIL_IF(c)(UTIL_EXPAND, UTIL_EAT)

#define UTIL_REPEAT(count, macro, ...)			      \
	UTIL_WHEN(count)				      \
	(						      \
		UTIL_OBSTRUCT(UTIL_REPEAT_INDIRECT) ()	      \
		(					      \
			UTIL_DEC(count), macro, __VA_ARGS__ \
		)					      \
		UTIL_OBSTRUCT(macro)			      \
		(					      \
			UTIL_DEC(count), __VA_ARGS__	      \
		)					      \
	)
#define UTIL_REPEAT_INDIRECT() UTIL_REPEAT

#define UTIL_LISTIFY(LEN, F, ...) UTIL_EVAL(UTIL_REPEAT(LEN, F, __VA_ARGS__))

extern int char2hex(char c, uint8_t *x);
extern int hex2char(uint8_t x, char *c);
extern uint8_t u8_to_dec(char *buf, uint8_t buflen, uint8_t value);
#endif

