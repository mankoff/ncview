/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993 through 2024 David W. Pierce
 *
 * This program  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * David W. Pierce
 * davidwilliampierce@gmail.com
 */

static int cmap_3gauss[] = {
	0,0,255, 0,0,255, 1,2,254, 2,4,253, 3,6,252, 4,9,251, 5,12,250, 6,16,249, 
	7,21,248, 8,26,247, 9,32,246, 10,39,245, 11,46,244, 12,53,243, 13,61,242, 14,69,241, 
	15,78,240, 16,87,239, 17,96,238, 18,105,237, 19,114,236, 20,123,235, 21,133,234, 22,142,233, 
	23,151,232, 24,160,231, 25,169,230, 26,178,229, 27,186,228, 28,194,227, 29,202,226, 30,209,225, 
	31,216,224, 32,223,223, 33,229,222, 34,234,221, 35,239,220, 36,243,219, 37,246,218, 38,249,217, 
	39,251,216, 40,253,215, 41,253,214, 42,253,213, 43,253,212, 44,252,211, 45,250,210, 46,247,209, 
	47,244,208, 48,240,207, 49,235,206, 50,230,205, 51,225,204, 52,218,203, 53,212,202, 54,205,201, 
	55,197,200, 56,189,199, 57,181,198, 58,172,197, 59,163,196, 60,154,195, 61,145,194, 62,136,193, 
	63,127,192, 64,117,191, 65,108,190, 66,99,189, 67,90,188, 68,81,187, 69,72,186, 70,64,185, 
	71,56,184, 72,48,183, 73,41,182, 74,35,181, 75,28,180, 76,23,179, 77,18,178, 78,13,177, 
	79,9,176, 80,6,175, 81,3,174, 82,1,173, 83,0,172, 84,0,171, 85,0,170, 86,0,169, 
	87,2,168, 88,4,167, 89,7,166, 90,10,165, 91,14,164, 92,19,163, 93,24,162, 94,30,161, 
	95,37,160, 96,44,159, 97,51,158, 98,59,157, 99,67,156, 100,75,155, 101,84,154, 102,93,153, 
	103,102,152, 104,111,151, 105,120,150, 106,130,149, 107,139,148, 108,148,147, 109,157,146, 110,166,145, 
	111,175,144, 112,184,143, 113,192,142, 114,200,141, 115,207,140, 116,214,139, 117,221,138, 118,227,137, 
	119,232,136, 120,237,135, 121,241,134, 122,245,133, 123,248,132, 124,250,131, 125,252,130, 126,253,129, 
	127,254,128, 128,253,127, 129,252,126, 130,250,125, 131,248,124, 132,245,123, 133,241,122, 134,237,121, 
	135,232,120, 136,227,119, 137,221,118, 138,214,117, 139,207,116, 140,200,115, 141,192,114, 142,184,113, 
	143,175,112, 144,166,111, 145,157,110, 146,148,109, 147,139,108, 148,130,107, 149,120,106, 150,111,105, 
	151,102,104, 152,93,103, 153,84,102, 154,75,101, 155,67,100, 156,59,99, 157,51,98, 158,44,97, 
	159,37,96, 160,30,95, 161,24,94, 162,19,93, 163,14,92, 164,10,91, 165,7,90, 166,4,89, 
	167,2,88, 168,0,87, 169,0,86, 170,0,85, 171,0,84, 172,1,83, 173,3,82, 174,6,81, 
	175,9,80, 176,13,79, 177,18,78, 178,23,77, 179,28,76, 180,35,75, 181,41,74, 182,48,73, 
	183,56,72, 184,64,71, 185,72,70, 186,81,69, 187,90,68, 188,99,67, 189,108,66, 190,117,65, 
	191,126,64, 192,136,63, 193,145,62, 194,154,61, 195,163,60, 196,172,59, 197,181,58, 198,189,57, 
	199,197,56, 200,205,55, 201,212,54, 202,218,53, 203,225,52, 204,230,51, 205,235,50, 206,240,49, 
	207,244,48, 208,247,47, 209,250,46, 210,252,45, 211,253,44, 212,253,43, 213,253,42, 214,253,41, 
	215,251,40, 216,249,39, 217,246,38, 218,243,37, 219,239,36, 220,234,35, 221,229,34, 222,223,33, 
	223,216,32, 224,209,31, 225,202,30, 226,194,29, 227,186,28, 228,178,27, 229,169,26, 230,160,25, 
	231,151,24, 232,142,23, 233,133,22, 234,123,21, 235,114,20, 236,105,19, 237,96,18, 238,87,17, 
	239,78,16, 240,69,15, 241,61,14, 242,53,13, 243,46,12, 244,39,11, 245,32,10, 246,26,9, 
	247,21,8, 248,16,7, 249,12,6, 250,8,5, 251,5,4, 252,3,3, 253,1,2, 255,0,1};
