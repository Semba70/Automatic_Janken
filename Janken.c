#include <stdio.h>
#include <stdlib.h>

#define HEIGHT 600
#define WIDTH  800

typedef unsigned char uchar;

typedef struct {
  int x;     // x 座標
  int y;     // y 座標
} IprPoint;

typedef struct {
  int max;         // スタックサイズ
  int ptr;         // スタックポインタ
  IprPoint *data;  // スタック（先頭要素へのポインタ)
} IprStack;

typedef struct {
  int label;
  int point_l[2];
  int point_r[2];
  int f_cnt ;
  char shape;
  char result;
} status;

// 二つの整数を受け取って，構造体に変換する関数
IprPoint iprPoint(int x, int y) {
  IprPoint p;
  p.x = x;
  p.y = y;
  return p;
}

uchar clip(double x)
{
  if (x >= 256) return 255;
  if (x <    0) return   0;
  return x;
}

int labeling(uchar binary[][WIDTH], int labelImage[][WIDTH]);

void detect_skin_region(int hsv[][WIDTH][3], uchar skin[][WIDTH],const int under[3], const int upper[3]);
void load_ppm(uchar image[][WIDTH][3], const char path[]);
void rgb2hsv(unsigned char rgb[][WIDTH][3], int hsv[][WIDTH][3]);
void dilate(uchar src[][WIDTH], uchar dst[][WIDTH], int iteration);
void erode(uchar src[][WIDTH], uchar dst[][WIDTH], int iteration);
void opening(uchar src[][WIDTH], uchar dst[][WIDTH], int iteration);
void closing(uchar src[][WIDTH], uchar dst[][WIDTH], int iteration);

void kukei(uchar skin_region[][WIDTH],int labels[][WIDTH],int nlabel,status *hand){

  int k;
  int l;
  int i;
  int x,y;
  int minX,minY,maxX,maxY;
  int count=0;


  for(k=1;k<=nlabel;k++){
    count=0;
    for (y = 0; y < HEIGHT; y++) {
      for (x = 0; x < WIDTH; x++) {
	l=labels[y][x];
	if(l==k){
	  count++;
	  if(count==1){
            minX=x;
            maxX=x;
            minY=y;
            maxY=y;
	  }
	  if(x<minX){  //各手の一番小さい、大きいxy座標を求める。
	    minX=x;
	  }
	  if(x>maxX){
	    maxX=x;
	  }
	  if(y<minY){
	    minY=y;
	  }
	  if(y>maxY){
	    maxY=y;
	  }


	}//同じラベルを見る

      }
    }
    hand[k-1].label = k;
    hand[k-1].point_l[0] = minX-1;
    hand[k-1].point_l[1] = minY-1;
    hand[k-1].point_r[0] = maxX+1;
    hand[k-1].point_r[1] = maxY+1;
    hand[k-1].f_cnt = 0;
    hand[k-1].shape = 'N';
    hand[k-1].result = 'N';

  } //手の数ループ
}



void hantei(unsigned char image[][WIDTH],int labels[][WIDTH],status s[], int nlabels){

  int flag, i, j, k, cnt;
  int line[4];

  for(k=0;k<nlabels;k++){

    flag=0;
    cnt=0;

    line[0]=s[k].point_l[0]+(s[k].point_r[0]-s[k].point_l[0])/4;
    line[1]=s[k].point_r[0]-(s[k].point_r[0]-s[k].point_l[0])/4;
    line[2]=s[k].point_l[1]+(s[k].point_r[1]-s[k].point_l[1])/4;
    line[3]=s[k].point_r[1]-(s[k].point_r[1]-s[k].point_l[1])/4;



    for(i=0;i<2;i++){
      for(j=s[k].point_l[1];j<=s[k].point_r[1];j++){
	if(image[j][line[i]] == 255 && flag == 0){
	  flag++;
	  cnt++;
	}
	if(image[j][line[i]] == 0 && flag == 1){
	  flag--;
	  cnt++;
	}
      }
      if(cnt>s[k].f_cnt){
	s[k].f_cnt =cnt;
      }
      cnt=0;
    }
    for(i=2;i<4;i++){
      for(j=s[k].point_l[0];j<=s[k].point_r[0];j++){
	if(image[line[i]][j] == 255 && flag == 0){
	  flag++;
	  cnt++;
	}
	if(image[line[i]][j] == 0 && flag == 1){
	  flag--;
	  cnt++;
	}
      }
      if(cnt>s[k].f_cnt){
	s[k].f_cnt =cnt;
      }
      cnt=0;
    }
    if(s[k].f_cnt>7){
      s[k].shape='p';
    }
    else if(s[k].f_cnt>3){
      s[k].shape='t';
    }
    else{
      s[k].shape='g';
    }

  }

}

void syohai(status s[], int n){
  int cnt_g=0, cnt_t=0, cnt_p=0, cnt, i;

  for(i=0;i<n;i++){
    if(s[i].shape=='g')cnt_g=1;
    else if(s[i].shape=='t')cnt_t=1;
    else if(s[i].shape=='p')cnt_p=1;
  }
  cnt = cnt_g + cnt_t + cnt_p;

  if(cnt==1 || cnt==3){
    for(i=0;i<n;i++){
      s[i].result='d';
    }
  }
  else{
    if(cnt_g && cnt_t){
      for(i=0;i<n;i++){
	if(s[i].shape=='g') s[i].result='w';
	else s[i].result='l';
      }
    }
    else if(cnt_g && cnt_p){
      for(i=0;i<n;i++){
	if(s[i].shape=='p') s[i].result='w';
	else s[i].result='l';
      }
    }
    else{
      for(i=0;i<n;i++){
	if(s[i].shape=='t') s[i].result='w';
	else s[i].result='l';
      }
    }
  }

}

void sort(status s[], int n){
  int i, j, label, min;
  int tmp;
  char str;

  for(j=0;j<n-1;j++){
    min = 800;
    for(i=j;i<n;i++){
      if(s[i].point_l[0]<min){
	label=i;
	min = s[i].point_l[0];
      }
    }
    tmp = s[j].point_l[0];
    s[j].point_l[0] = s[label].point_l[0];
    s[label].point_l[0] = tmp;
    tmp = s[j].point_l[1];
    s[j].point_l[1] = s[label].point_l[1];
    s[label].point_l[1] = tmp;

    tmp = s[j].point_r[0];
    s[j].point_r[0] = s[label].point_r[0];
    s[label].point_r[0] = tmp;
    tmp = s[j].point_r[1];
    s[j].point_r[1] = s[label].point_r[1];
    s[label].point_r[1] = tmp;

    str = s[j].shape;
    s[j].shape = s[label].shape;
    s[label].shape = str;

    str = s[j].result;
    s[j].result = s[label].result;
    s[label].result = str;
  }

}

int main(int argc, char *argv[])
{
  static uchar rgb[HEIGHT][WIDTH][3];
  static int hsv[HEIGHT][WIDTH][3];
  int x,y,l;
  status hand[10];

  // PPM 画像を入力する
  load_ppm(rgb, argv[1]);

  // RGB ==> HSV に変換
  rgb2hsv(rgb, hsv);

  int under[3], upper[3];     // 肌色領域の下限と上限
  /////
  // 赤色領域を肌色として抽出します．
  // 赤色は Hue が 0 度を中心に分布するので，
  // 320 〜 360 と 0 〜 40 の二つの領域に分けて抽出したあと，
  // 二つの領域を併合（or 演算）しています．
  /////

  // １つ目（0 <= Hue <= 40）
  under[0] =  0; upper[0] = 40;   // 色相の上限と下限
  under[1] = 20; upper[1] = 100;   // 彩度の上限と下限
  under[2] = 20; upper[2] = 100;   // 明度の上限と下限
  uchar tmp1[HEIGHT][WIDTH];
  detect_skin_region(hsv, tmp1, under, upper);

  // ２つ目（320 <= Hue <= 360）
  under[0] = 320; upper[0] = 360;
  under[1] =  20; upper[1] =  100;
  under[2] =  20; upper[2] =  100;
  static uchar tmp2[HEIGHT][WIDTH];
  detect_skin_region(hsv, tmp2, under, upper);

  // or 演算でマージ
  static uchar skin_region[HEIGHT][WIDTH];
  for (y = 0; y < HEIGHT; y++) {
    for ( x = 0; x < WIDTH; x++) {
      skin_region[y][x] = tmp1[y][x] | tmp2[y][x];
    }
  }

  //オープンクローズ

  int i;

  opening(skin_region, skin_region,1);
  closing(skin_region, skin_region,2);
  opening(skin_region, skin_region,1);


  static int labels[HEIGHT][WIDTH];
  int nlabel = labeling(skin_region, labels);  // nlabel はラベル数

  kukei(skin_region,labels,nlabel,hand);

  hantei(skin_region,labels,hand,nlabel);

  syohai(hand,nlabel);

  sort(hand,nlabel);

  for(i=0;i<nlabel;i++){
    printf("%d %d ",hand[i].point_l[0]-1,hand[i].point_l[1]-1);
    printf("%d %d ",hand[i].point_r[0]-hand[i].point_l[0]-2,
	   hand[i].point_r[0]-hand[i].point_l[1]-2);
    printf("%c ",hand[i].shape);
    printf("%c\n",hand[i].result);
  }

  //save_pgm(skin_region,argv[2]);
  return 0;
}


int StackAlloc(IprStack *s, int max)
{
  s->ptr = 0;
  s->data = (IprPoint *) calloc(max, sizeof(IprPoint));
  if (s->data == NULL) {
    fprintf(stderr, "スタックの確保に失敗しました．\n");
    s->max = 0;
    return -1;
  }
  s->max = max;

  return 0;
}

void StackFree(IprStack *s)
{
  if (s->data != NULL) {
    free(s->data);
    s->max = s->ptr = 0;
  }
}

int StackPush(IprStack *s, IprPoint x)
{
  if (s->ptr >= s->max) {
    fprintf(stderr, "スタックが一杯です．\n");
    return -1;
  }

  s->data[s->ptr++] = x;

  return 0;
}

int StackPop(IprStack *s, IprPoint *x)
{
  if (s->ptr <= 0) {
    fprintf(stderr, "スタックは空です．\n");
    return -1;
  }

  *x = s->data[--s->ptr];

  return 0;
}

int StackIsEmpty(IprStack *s)
{
  return (s->ptr <= 0);
}

int labeling(uchar binary[][WIDTH], int labelImage[][WIDTH])
{
  int x, y, n, xx, yy, a=0;
  IprStack stack;
  StackAlloc(&stack, HEIGHT * WIDTH);
  IprPoint neighbor[8] = {
    { 1, 0}, { 1, -1}, { 0, -1}, {-1, -1},
    {-1, 0}, {-1,  1}, { 0,  1}, { 1,  1}};

  // ラベルとラベル画像の初期化
  int label = 0;
  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      labelImage[y][x] = 0;
    }
  }

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {

      if (binary[y][x] != 0 && labelImage[y][x] == 0) {
	label++;
	labelImage[y][x] = label;
	StackPush(&stack, iprPoint(x, y));

	while (!StackIsEmpty(&stack)) {
	  IprPoint p;
	  StackPop(&stack, &p);
	  for (n = 0; n < 8; n++) {
	    xx = p.x + neighbor[n].x;
	    yy = p.y + neighbor[n].y;
	    if (xx >= 0 && xx < WIDTH &&
		yy >= 0 && yy < HEIGHT &&
		binary[yy][xx] != 0 &&
		labelImage[yy][xx] == 0)
	      {
		labelImage[yy][xx] = label;
		StackPush(&stack, iprPoint(xx, yy));
		a++;
	      }
	  }
	}
	if(a < 10){
	  label--;
	  a=0;
	}
      }
    }
  }

  return label;
}

void load_pgm(unsigned char image[][WIDTH], const char path[])
{
  char magic_number[2];
  int width, height;
  int max_intensity;
  FILE *fp;

  fp = fopen(path, "rb");
  if (fp == NULL) {
    fprintf(stderr, "%s が開けませんでした．\n", path);
    exit(1);
  }

  fscanf(fp, "%c%c", &magic_number[0], &magic_number[1]);
  if (magic_number[0] != 'P' || magic_number[1] != '5') {
    fprintf(stderr, "%s はバイナリ型 PGM ではありません．\n", path);
    fclose(fp);
    exit(1);
  }

  fscanf(fp, "%d %d", &width, &height);
  if (width != WIDTH || height != HEIGHT) {
    fprintf(stderr, "画像のサイズが異なります．\n");
    fprintf(stderr, "  想定サイズ：WIDTH = %d, HEIGHT = %d\n", WIDTH, HEIGHT);
    fprintf(stderr, "  実サイズ：  width = %d, height = %d\n", width, height);
    fclose(fp);
    exit(1);
  }

  fscanf(fp, "%d", &max_intensity);
  if (max_intensity != 255) {
    fprintf(stderr, "最大階調値が不正な値です（%d）．\n", max_intensity);
    fclose(fp);
    exit(1);
  }

  fgetc(fp);  // 最大階調値の直後の改行コードを読み捨て

  fread(image, sizeof(unsigned char), HEIGHT * WIDTH, fp);

  fclose(fp);
}

void save_pgm(unsigned char image[][WIDTH], const char path[])
{
  FILE *fp;

  fp = fopen(path, "wb");
  if (fp == NULL) {
    fprintf(stderr, "%s が開けませんでした．\n", path);
    exit(1);
  }

  fprintf(fp, "P5\n");
  fprintf(fp, "%d %d\n", WIDTH, HEIGHT);
  fprintf(fp, "255\n");
  fwrite(image, sizeof(unsigned char), HEIGHT * WIDTH, fp);

  fclose(fp);
}

void load_ppm(uchar image[][WIDTH][3], const char path[])
{
  char magic_number[2];
  int width, height;
  int max_intensity;
  FILE *fp;

  fp = fopen(path, "rb");
  if (fp == NULL) {
    fprintf(stderr, "%s が開けませんでした．\n", path);
    exit(1);
  }

  fscanf(fp, "%c%c", &magic_number[0], &magic_number[1]);
  if (magic_number[0] != 'P' || magic_number[1] != '6') {
    fprintf(stderr, "%s はバイナリ型 PPM ではありません．\n", path);
    fclose(fp);
    exit(1);
  }

  fscanf(fp, "%d %d", &width, &height);
  if (width != WIDTH || height != HEIGHT) {
    fprintf(stderr, "画像のサイズが異なります．\n");
    fprintf(stderr, "  想定サイズ：WIDTH = %d, HEIGHT = %d\n", WIDTH, HEIGHT);
    fprintf(stderr, "  実サイズ：  width = %d, height = %d\n", width, height);
    fclose(fp);
    exit(1);
  }

  fscanf(fp, "%d", &max_intensity);
  if (max_intensity != 255) {
    fprintf(stderr, "最大階調値が不正な値です（%d）．\n", max_intensity);
    fclose(fp);
    exit(1);
  }

  fgetc(fp);  // 最大階調値の直後の改行コードを読み捨て

  fread(image, sizeof(uchar), HEIGHT * WIDTH * 3, fp);

  fclose(fp);
}

void save_ppm(uchar image[][WIDTH][3], const char path[])
{
  FILE *fp;

  fp = fopen(path, "wb");
  if (fp == NULL) {
    fprintf(stderr, "%s が開けませんでした．\n", path);
    exit(1);
  }

  fprintf(fp, "P6\n");
  fprintf(fp, "%d %d\n", WIDTH, HEIGHT);
  fprintf(fp, "255\n");
  fwrite(image, sizeof(uchar), HEIGHT * WIDTH * 3, fp);

  fclose(fp);
}

int max3(unsigned char r, unsigned char g, unsigned char b)
{
  int max;

  if (r > g) {
    if (r > b) {
      max = r;
    }
    else {
      max = b;
    }
  }
  else {
    if (g > b) {
      max = g;
    }
    else {
      max = b;
    }
  }

  return max;
}

int min3(unsigned char r, unsigned char g, unsigned char b)
{
  int min;

  if (r < g) {
    if (r < b) {
      min = r;
    }
    else {
      min = b;
    }
  }
  else {
    if (g < b) {
      min = g;
    }
    else {
      min = b;
    }
  }

  return min;
}

void rgb2hsv(unsigned char rgb[][WIDTH][3], int hsv[][WIDTH][3])
{
  int Vmin, Vmax;
  int m, n;
  double den;

  for (m = 0; m < HEIGHT; m++) {
    for (n = 0; n < WIDTH; n++) {
      Vmax = max3(rgb[m][n][0], rgb[m][n][1], rgb[m][n][2]);
      Vmin = min3(rgb[m][n][0], rgb[m][n][1], rgb[m][n][2]);

      hsv[m][n][2] = 100 * Vmax / 255.0;

      if (Vmax == 0) {
	hsv[m][n][1] = 0;
      }
      else {
	hsv[m][n][1] = 100.0 * (Vmax - Vmin) / (double) Vmax;
      }

      if (Vmax == Vmin) {
	hsv[m][n][0] = -1;
      }
      else {
	den = Vmax - Vmin;
	if (Vmax == rgb[m][n][0]) {
	  hsv[m][n][0] = 60.0 * (rgb[m][n][1] - rgb[m][n][2]) / den;
	}
	else if (Vmax == rgb[m][n][1]) {
	  hsv[m][n][0] = 60.0 * (rgb[m][n][2] - rgb[m][n][0]) / den + 120;
	}
	else {
	  hsv[m][n][0] = 60.0 * (rgb[m][n][0] - rgb[m][n][2]) / den + 240;
	}

	if (hsv[m][n][0] < 0) {
	  hsv[m][n][0] += 360;
	}

	if (hsv[m][n][0] >= 360) {
	  hsv[m][n][0] -= 360;
	}
      }
    }
  }
}

void detect_skin_region(int hsv[][WIDTH][3], uchar skin[][WIDTH],  const int under[3], const int upper[3])
{
  int x, y;
  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {

      if (under[0] <= hsv[y][x][0] && hsv[y][x][0] < upper[0] &&
	  under[1] <= hsv[y][x][1] && hsv[y][x][1] < upper[1] &&
	  under[2] <= hsv[y][x][2] && hsv[y][x][2] < upper[2] ) {
	skin[y][x] = 255;
      }
      else {
	skin[y][x] = 0;
      }

    }
  }
}

void opening(uchar src[][WIDTH], uchar dst[][WIDTH], int iteration)
{
  uchar tmp[HEIGHT][WIDTH] = {{0}};
  erode (src, tmp, iteration);
  dilate(tmp, dst, iteration);
}

void closing(uchar src[][WIDTH], uchar dst[][WIDTH], int iteration)
{
  uchar tmp[HEIGHT][WIDTH] = {{0}};
  dilate(src, tmp, iteration);
  erode (tmp, dst, iteration);
}

void copy_image(uchar src[][WIDTH], uchar dst[][WIDTH])
{
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      dst[y][x] = src[y][x];
    }
  }
}


void dilate(uchar src[][WIDTH], uchar dst[][WIDTH], int iteration)
{
  uchar tmp[HEIGHT][WIDTH];

  copy_image(src, tmp);

  for (int i = 0; i < iteration; i++) {

    /////
    // 四隅の処理
    /////

    // (0, 0)
    if (tmp[0][0] != 0 || tmp[0][1] != 0 ||
        tmp[1][0] != 0 || tmp[1][1] != 0) {
      dst[0][0] = 255;
    }
    else {
      dst[0][0] = 0;
    }

    // (0, WIDTH - 1)
    if (tmp[0][WIDTH - 2] != 0 || tmp[0][WIDTH - 1] != 0 ||
        tmp[1][WIDTH - 2] != 0 || tmp[1][WIDTH - 1] != 0) {
      dst[0][WIDTH - 1] = 255;
    }
    else {
      dst[0][WIDTH - 1] = 0;
    }

    // (HEIGHT - 1, 0)
    if (tmp[HEIGHT - 2][0] != 0 || tmp[HEIGHT - 2][1] != 0 ||
        tmp[HEIGHT - 1][0] != 0 || tmp[HEIGHT - 1][1] != 0) {
      dst[HEIGHT - 1][0] = 255;
    }
    else {
      dst[HEIGHT - 1][WIDTH - 1] = 0;
    }

    // (HEIGHT - 1, WIDTH - 1)
    if (tmp[HEIGHT - 2][WIDTH - 2] != 0 || tmp[HEIGHT - 2][WIDTH - 1] != 0 ||
        tmp[HEIGHT - 1][WIDTH - 2] != 0 || tmp[HEIGHT - 1][WIDTH - 1] != 0) {
      dst[HEIGHT - 1][WIDTH - 1] = 255;
    }
    else {
      dst[HEIGHT - 1][WIDTH - 1] = 0;
    }
    /////
    // 四隅の処理はここまで
    /////

    /////
    // 四隅を除く端の処理
    /////

    // 上端 (0, 1) -- (0, WIDTN - 2)
    for (int x = 1; x < WIDTH - 2; x++) {
      if (tmp[0][x - 1] != 0 || tmp[0][x] != 0 || tmp[0][x + 1] != 0 ||
          tmp[1][x - 1] != 0 || tmp[1][x] != 0 || tmp[1][x + 1] != 0) {
        dst[0][x] = 255;
      }
      else {
        dst[0][x] = 0;
      }
    }

    // 下端 (HEIGHT - 1, 1) -- (HEIGHT - 1, WIDTH - 2)
    for (int x = 1; x < WIDTH - 2; x++) {
      if (tmp[HEIGHT - 2][x - 1] != 0 || tmp[HEIGHT - 2][x] != 0 || tmp[HEIGHT - 2][x + 1] != 0 ||
          tmp[HEIGHT - 1][x - 1] != 0 || tmp[HEIGHT - 1][x] != 0 || tmp[HEIGHT - 1][x + 1] != 0) {
        dst[HEIGHT - 1][x] = 255;
      }
      else {
        dst[HEIGHT - 1][x] = 0;
      }
    }

    // 左端 (1, 0) -- (HEIGHT - 2, 0)
    for (int y = 1; y < HEIGHT - 2; y++) {
      if (tmp[y - 1][0] != 0 || tmp[y - 1][1] != 0 ||
	  tmp[y    ][0] != 0 || tmp[y    ][1] != 0 ||
          tmp[y + 1][0] != 0 || tmp[y + 1][1] != 0) {
        dst[y][0] = 255;
      }
      else {
        dst[y][0] = 0;
      }
    }

    // 右端 (1, WIDTH - 1) -- (HEIGHT - 2, WIDTH - 1)
    for (int y = 1; y < HEIGHT - 2; y++) {
      if (tmp[y - 1][WIDTH - 2] != 0 || tmp[y - 1][WIDTH - 1] != 0 ||
	  tmp[y    ][WIDTH - 2] != 0 || tmp[y    ][WIDTH - 1] != 0 ||
          tmp[y + 1][WIDTH - 2] != 0 || tmp[y + 1][WIDTH - 1] != 0) {
        dst[y][WIDTH - 1] = 255;
      }
      else {
        dst[y][WIDTH - 1] = 0;
      }
    }
    /////
    // 端の処理はここまで
    /////

    for (int y = 1; y < HEIGHT - 1; y++) {
      for (int x = 1; x < WIDTH - 1; x++) {
        if (tmp[y - 1][x - 1] != 0 || tmp[y - 1][x] != 0 || tmp[y - 1][x + 1] != 0 ||
	    tmp[y    ][x - 1] != 0 || tmp[y    ][x] != 0 || tmp[y    ][x + 1] != 0 ||
            tmp[y + 1][x - 1] != 0 || tmp[y + 1][x] != 0 || tmp[y + 1][x + 1] != 0 ) {
          dst[y][x] = 255;
        }
        else {
          dst[y][x] = 0;
        }
      }
    }

    copy_image(dst, tmp);
  }
}

void erode(uchar src[][WIDTH], uchar dst[][WIDTH], int iteration)
{
  uchar tmp[HEIGHT][WIDTH];

  copy_image(src, tmp);

  for (int i = 0; i < iteration; i++) {

    /////
    // 四隅の処理
    /////

    // (0, 0)
    if (tmp[0][0] == 0 || tmp[0][1] == 0 ||
        tmp[1][0] == 0 || tmp[1][1] == 0) {
      dst[0][0] = 0;
    }
    else {
      dst[0][0] = 255;
    }

    // (0, WIDTH - 1)
    if (tmp[0][WIDTH - 2] == 0 || tmp[0][WIDTH - 1] == 0 ||
        tmp[1][WIDTH - 2] == 0 || tmp[1][WIDTH - 1] == 0) {
      dst[0][WIDTH - 1] = 0;
    }
    else {
      dst[0][WIDTH - 1] = 255;
    }

    // (HEIGHT - 1, 0)
    if (tmp[HEIGHT - 2][0] == 0 || tmp[HEIGHT - 2][1] == 0 ||
        tmp[HEIGHT - 1][0] == 0 || tmp[HEIGHT - 1][1] == 0) {
      dst[HEIGHT - 1][0] = 0;
    }
    else {
      dst[HEIGHT - 1][WIDTH - 1] = 255;
    }

    // (HEIGHT - 1, WIDTH - 1)
    if (tmp[HEIGHT - 2][WIDTH - 2] == 0 || tmp[HEIGHT - 2][WIDTH - 1] == 0 ||
        tmp[HEIGHT - 1][WIDTH - 2] == 0 || tmp[HEIGHT - 1][WIDTH - 1] == 0) {
      dst[HEIGHT - 1][WIDTH - 1] = 0;
    }
    else {
      dst[HEIGHT - 1][WIDTH - 1] = 255;
    }
    /////
    // 四隅の処理はここまで
    /////

    /////
    // 四隅を除く端の処理
    /////

    // 上端 (0, 1) -- (0, WIDTN - 2)
    for (int x = 1; x < WIDTH - 2; x++) {
      if (tmp[0][x - 1] == 0 || tmp[0][x] == 0 || tmp[0][x + 1] == 0 ||
          tmp[1][x - 1] == 0 || tmp[1][x] == 0 || tmp[1][x + 1] == 0) {
        dst[0][x] = 0;
      }
      else {
        dst[0][x] = 255;
      }
    }

    // 下端 (HEIGHT - 1, 1) -- (HEIGHT - 1, WIDTH - 2)
    for (int x = 1; x < WIDTH - 2; x++) {
      if (tmp[HEIGHT - 2][x - 1] == 0 || tmp[HEIGHT - 2][x] == 0 || tmp[HEIGHT - 2][x + 1] == 0 ||
          tmp[HEIGHT - 1][x - 1] == 0 || tmp[HEIGHT - 1][x] == 0 || tmp[HEIGHT - 1][x + 1] == 0) {
        dst[HEIGHT - 1][x] = 0;
      }
      else {
        dst[HEIGHT - 1][x] = 255;
      }
    }

    // 左端 (1, 0) -- (HEIGHT - 2, 0)
    for (int y = 1; y < HEIGHT - 2; y++) {
      if (tmp[y - 1][0] == 0 || tmp[y - 1][1] == 0 ||
	  tmp[y    ][0] == 0 || tmp[y    ][1] == 0 ||
          tmp[y + 1][0] == 0 || tmp[y + 1][1] == 0) {
        dst[y][0] = 0;
      }
      else {
        dst[y][0] = 255;
      }
    }

    // 右端 (1, WIDTH - 1) -- (HEIGHT - 2, WIDTH - 1)
    for (int y = 1; y < HEIGHT - 2; y++) {
      if (tmp[y - 1][WIDTH - 2] == 0 || tmp[y - 1][WIDTH - 1] == 0 ||
	  tmp[y    ][WIDTH - 2] == 0 || tmp[y    ][WIDTH - 1] == 0 ||
          tmp[y + 1][WIDTH - 2] == 0 || tmp[y + 1][WIDTH - 1] == 0) {
        dst[y][WIDTH - 1] = 0;
      }
      else {
        dst[y][WIDTH - 1] = 255;
      }
    }
    /////
    // 端の処理はここまで
    /////

    for (int y = 1; y < HEIGHT - 1; y++) {
      for (int x = 1; x < WIDTH - 1; x++) {
        if (tmp[y - 1][x - 1] == 0 || tmp[y - 1][x] == 0 || tmp[y - 1][x + 1] == 0 ||
	    tmp[y    ][x - 1] == 0 || tmp[y    ][x] == 0 || tmp[y    ][x + 1] == 0 ||
            tmp[y + 1][x - 1] == 0 || tmp[y + 1][x] == 0 || tmp[y + 1][x + 1] == 0 ) {
          dst[y][x] = 0;
        }
        else {
          dst[y][x] = 255;
        }
      }
    }

    copy_image(dst, tmp);
  }
}
