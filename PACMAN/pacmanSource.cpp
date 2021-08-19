#include <time.h>
#include <math.h>
#include "glut.h"
#include "glm/gtc/constants.hpp"

#define WINDOW_HEIGHT		(702)

#define BLOCK_ROW_MAX		(27)
#define BLOCK_COLUMN_MAX	(21)

#define BLOCK_HEIGHT		(WINDOW_HEIGHT / BLOCK_ROW_MAX)	//26
#define BLOCK_WIDTH			(BLOCK_HEIGHT)					//26

#define WINDOW_WIDTH		(BLOCK_WIDTH * BLOCK_COLUMN_MAX)//546

#define PACMAN_SPEED		(3)

#define MONSTER_SPEED		(2)

//ブロックの種類
enum {
	BLOCK_TYPE_WALL,
	BLOCK_TYPE_DOT,
	BLOCK_TYPE_POWER,
	BLOCK_TYPE_NONE,
	BLOCK_TYPE_NAX
};

//キャラクターの種類
enum {
	CHARACTER_PACMAN,
	CHARACTER_RED,
	CHARACTER_BLUE,
	CHARACTER_PINK,
	CHARACTER_ORANGE,
	CHARACTER_MAX
};

//時間の種類
enum {
	CLOCK_GO_OUT,
	CLOCK_INVINCIBLE,
	CLOCK_CHASE_MODE,
	CLOCK_TERRITORY_MODE,
	CLOCK_UPDATE,
	CLOCK_MAX
};

//方角
enum {
	DIRECTION_NORTH,
	DIRECTION_WEST,
	DIRECTION_SOUTH,
	DIRECTION_EAST,
	DIRECTION_MAX
};

//方角ごとの進む距離
int directions[][2] = {
	{0, -1},	//DIRECTION_NORTH,
	{-1, 0},	//DIRECTION_WEST,
	{0, 1},		//DIRECTION_SOUTH,
	{1, 0},		//DIRECTION_EAST,
};

//迷路内のブロックは27×21個あり、1ブロックの幅と高さは26
char mapData[] = "\
#####################\
#ooooooooo#ooooooooo#\
#o###o###o#o###o###o#\
#0# #o# #o#o# #o# #0#\
#o###o###o#o###o###o#\
#ooooooooooooooooooo#\
#o###o#o#####o#o###o#\
#o###o#o#####o#o###o#\
#ooooo#ooo#ooo#ooooo#\
#####o### # ###o#####\
    #o#   r   #o#    \
    #o# ##### #o#    \
#####o# #bpy# #o#####\
     o  #   #  o     \
#####o# ##### #o#####\
    #o#       #o#    \
    #o# ##### #o#    \
#####o# ##### #o#####\
#ooooooooo#ooooooooo#\
#o###o###o#o###o###o#\
#0oo#ooooo@ooooo#oo0#\
###o#o#o#####o#o#o###\
###o#o#o#####o#o#o###\
#ooooo#ooo#ooo#ooooo#\
#o#######o#o#######o#\
#ooooooooooooooooooo#\
#####################";

//2次元ベクトル
typedef struct {
	int x, y;
}VEC2;

//キャラクターごとの縄張り地点
VEC2 territoryPosition[CHARACTER_MAX] = {
	{-1, -1},	//CHARACTER_PACMAN,
	{19, 1},	//CHARACTER_RED,
	{19, 25},	//CHARACTER_BLUE,
	{1, 1},		//CHARACTER_PINK,
	{1, 25},	//CHARACTER_ORANGE,
};

//フラグ
typedef struct {
	bool init;			//プログラム初期化
	bool invincibleMode;//パックマン無敵
	bool territoryMode;	//縄張りモード
	bool chaseMode;		//追跡モード
}FLAG;

FLAG flag;

//時間制御
typedef struct {
	clock_t lastClock;	//最後の計測時間
	clock_t nowClock;	//最新の計測時間
	float interval;		//時間間隔
}CLOCK;

CLOCK clockMode[CLOCK_MAX];

//ブロック
typedef struct {
	int type;					//ブロックの種類
	bool checked[CHARACTER_MAX];//キャラクターごとの移動したフラグ
}BLOCK;

BLOCK blocks[BLOCK_ROW_MAX][BLOCK_COLUMN_MAX];

//キャラクター
typedef struct {
	VEC2 position;			//座標
	VEC2 lastPosition;		//古い座標
	bool invincible;		//無敵フラグ
	bool stayHome;			//巣待機フラグ　モンスターのみ
	bool goHome;			//巣帰宅フラグ　モンスターのみ
	bool moveAnimation;		//口の開閉　パックマンのみ
	int directionOfMovement;//進行方向
	int monsterSpeed;		//速さ
	int dotAndPowerCount;	//カウント　パックマンのみ
}CHARACTER;

CHARACTER characters[CHARACTER_MAX];

//モンスターが待機する巣の前の座標
int inFrontOfNestPointX;
int inFrontOfNestPointY;

//キーボード256個分のバイト配列
bool keysPressed[256];

//目的地に向かう途中でどこにも進めなくなれば、ルートを初期化する処理
void routeInit(int _characterNumber) {
	for (int y = 0;y < BLOCK_ROW_MAX;y++)
		for (int x = 0;x < BLOCK_COLUMN_MAX;x++)
			if (blocks[y][x].checked[_characterNumber])
				blocks[y][x].checked[_characterNumber] = false;

	characters[_characterNumber].directionOfMovement = -1;
}

//プログラム開始直後に初期化する処理
void Init(void) {
	//文字列mapDataから一つずつブロックの種類を分ける、その際キャラクターの初期位置も取得する
	for (int y = 0;y < BLOCK_ROW_MAX;y++) {
		for (int x = 0;x < BLOCK_COLUMN_MAX;x++) {
			char c = mapData[y * BLOCK_COLUMN_MAX + x];
			switch (c) {
			case '#': blocks[y][x].type = BLOCK_TYPE_WALL;	break;
			case 'o': blocks[y][x].type = BLOCK_TYPE_DOT;	break;
			case '0': blocks[y][x].type = BLOCK_TYPE_POWER;	break;
			default:
				blocks[y][x].type = BLOCK_TYPE_NONE;
				switch (c) {
				case '@': characters[CHARACTER_PACMAN].position = { x,y };	break;
				case 'r': characters[CHARACTER_RED].position = { x,y };
						inFrontOfNestPointX = x;
						inFrontOfNestPointY = y;
						break;
				case 'b': characters[CHARACTER_BLUE].position = { x,y };	break;
				case 'p': characters[CHARACTER_PINK].position = { x,y };	break;
				case 'y': characters[CHARACTER_ORANGE].position = { x,y };	break;
				}
				break;
			}
		}
	}

	//迷路中央の入り口前の座標
	inFrontOfNestPointX = inFrontOfNestPointX * BLOCK_WIDTH + BLOCK_WIDTH / 2;
	inFrontOfNestPointY = inFrontOfNestPointY * BLOCK_HEIGHT + BLOCK_HEIGHT / 2;

	//縄張りモードから開始する、それ以外はフラグを下ろす
	flag.init = false;
	flag.territoryMode = true;
	flag.chaseMode = false;
	flag.invincibleMode = false;

	//キャラクターの状態を初期化
	for (int i = CHARACTER_PACMAN;i < CHARACTER_MAX;i++) {
		if (i == CHARACTER_PACMAN) {
			characters[i].invincible = false;
			characters[i].stayHome = false;
			characters[i].monsterSpeed = PACMAN_SPEED;

			//迷路内のドットとパワーエサを数える
			characters[i].dotAndPowerCount = 0;
			for (int y = 0;y < BLOCK_ROW_MAX;y++)
				for (int x = 0;x < BLOCK_COLUMN_MAX;x++)
					if ((blocks[y][x].type == BLOCK_TYPE_DOT) || (blocks[y][x].type == BLOCK_TYPE_POWER))
						characters[i].dotAndPowerCount++;
		}
		else {
			characters[i].invincible = true;
			characters[i].dotAndPowerCount = -1;
			characters[i].monsterSpeed = MONSTER_SPEED;
			if (i == CHARACTER_RED) characters[i].stayHome = false;
			else characters[i].stayHome = true;
		}

		//キャラクターの初期座標
		characters[i].position.x = characters[i].position.x * BLOCK_WIDTH + BLOCK_WIDTH / 2;
		characters[i].position.y = characters[i].position.y * BLOCK_HEIGHT + BLOCK_HEIGHT / 2;

		characters[i].moveAnimation = false;
		characters[i].goHome = false;
		characters[i].directionOfMovement = -1;
		routeInit(i);
	}

	//時間間隔の初期化
	clockMode[CLOCK_GO_OUT].interval = 5000;
	clockMode[CLOCK_INVINCIBLE].interval = 10000;
	clockMode[CLOCK_CHASE_MODE].interval = 20000;
	clockMode[CLOCK_TERRITORY_MODE].interval = 10000;
	clockMode[CLOCK_UPDATE].interval = 1000 / 60;

	//モンスターの、巣からの脱出時刻と縄張りモードの開始時刻を取得する
	clockMode[CLOCK_GO_OUT].lastClock = clock();
	clockMode[CLOCK_TERRITORY_MODE].lastClock = clock();
}

//追跡モードや縄張りモードなど、時間経過を管理する処理
bool timePasses(int _clockNumber) {
	//現在の時刻(_nowClock)が最後に測った時刻(_lastClock)から_interval秒経過していればtrueを渡す
	clockMode[_clockNumber].nowClock = clock();
	if (clockMode[_clockNumber].nowClock >= clockMode[_clockNumber].lastClock + clockMode[_clockNumber].interval)
		return true;
	else return false;
}

//ブロックの中央にいるかの判定処理、trueなら進路変更できる
bool isOnThePoint(int _characterNumber) {
	int x = characters[_characterNumber].position.x / BLOCK_WIDTH;
	int y = characters[_characterNumber].position.y / BLOCK_HEIGHT;

	if ((characters[_characterNumber].position.x == x * BLOCK_WIDTH + BLOCK_WIDTH / 2) &&
		(characters[_characterNumber].position.y == y * BLOCK_HEIGHT + BLOCK_HEIGHT / 2))
		return true;

	else return false;
}

//左右の通路を通過した際のワープ処理
void overflowFromMap(int _characterNumber) {
	if (characters[_characterNumber].position.x > WINDOW_WIDTH + BLOCK_WIDTH / 2)
		characters[_characterNumber].position.x = -BLOCK_WIDTH / 2;

	else if (characters[_characterNumber].position.x < -BLOCK_WIDTH / 2)
		characters[_characterNumber].position.x = WINDOW_WIDTH + BLOCK_WIDTH / 2;
}

//進んだブロックにキャラクターがいれば、その番号を取得できる
int getCharacter(int _x, int _y) {
	for (int i = CHARACTER_PACMAN;i < CHARACTER_MAX;i++) {

		int characterPosX = characters[i].position.x / BLOCK_WIDTH;
		int characterPosY = characters[i].position.y / BLOCK_HEIGHT;

		if ((_x == characterPosX) && (_y == characterPosY))
			return i;
	}
	return -1;
}

//現在の座標と前回の座標をもとに、進行方向を取得する
void getDirectionOfMovement(int _characterNumber) {
	VEC2 directionOfMovement = {
		characters[_characterNumber].position.x - characters[_characterNumber].lastPosition.x,
		characters[_characterNumber].position.y - characters[_characterNumber].lastPosition.y,
	};

	if (directionOfMovement.x != 0) {
		if (directionOfMovement.x > 0)
			characters[_characterNumber].directionOfMovement = DIRECTION_EAST;
		if (directionOfMovement.x < 0)
			characters[_characterNumber].directionOfMovement = DIRECTION_WEST;
	}

	if (directionOfMovement.y != 0) {
		if (directionOfMovement.y > 0)
			characters[_characterNumber].directionOfMovement = DIRECTION_SOUTH;
		if (directionOfMovement.y < 0)
			characters[_characterNumber].directionOfMovement = DIRECTION_NORTH;
	}
}

//パックマンとモンスターが衝突した際の処理
void crashBetweenCharacters(int _characterNumber) {
	/*
		モンスターがイジケてるなら、ブロックの中心に移動してからルートを初期化し巣に帰るフラグを立てて、
		通常の3倍の速さで巣に帰る。その際はキャラクター同士が衝突してもすり抜ける。
	*/
	if (!characters[_characterNumber].invincible)
	{
		int x = characters[_characterNumber].position.x / BLOCK_WIDTH;
		int y = characters[_characterNumber].position.y / BLOCK_HEIGHT;

		characters[_characterNumber].position.x = x * BLOCK_WIDTH + BLOCK_WIDTH / 2;
		characters[_characterNumber].position.y = y * BLOCK_HEIGHT + BLOCK_HEIGHT / 2;
		routeInit(_characterNumber);

		characters[_characterNumber].goHome = true;
		characters[_characterNumber].monsterSpeed = MONSTER_SPEED * 3;
	}
	//モンスターがイジケてないなら、プログラム全体の初期化フラグを立てる
	else flag.init = true;
}

//モンスターが移動中に味方同士で衝突するなら、進路変更する処理
void changeDirectionOfMovement(int _characterNumber) {
	//モンスターが巣に戻る途中なら処理しない。つまり衝突してもすり抜ける
	if (characters[_characterNumber].goHome) return;

	int x = characters[_characterNumber].position.x;
	int y = characters[_characterNumber].position.y;

	int directionOfMovement = characters[_characterNumber].directionOfMovement;

	//モンスターの東西南北の範囲を初期化
	int characterNorth = y;
	int characterWest = x;
	int characterSouth = y;
	int characterEast = x;

	//衝突範囲を選定。範囲は現在の位置から進行方向へ、ブロックの幅(高さ)の分までの直線上
	for (int j = 0;j < DIRECTION_MAX;j++) {
		if (j == (directionOfMovement + 2) % DIRECTION_MAX) continue;

		if (directionOfMovement == DIRECTION_NORTH) characterNorth = y - BLOCK_HEIGHT;
		if (directionOfMovement == DIRECTION_WEST) characterWest = x - BLOCK_WIDTH;
		if (directionOfMovement == DIRECTION_SOUTH) characterSouth = y + BLOCK_HEIGHT;
		if (directionOfMovement == DIRECTION_EAST) characterEast = x + BLOCK_WIDTH;
	}

	//進行方向の範囲内にキャラクターがいるか調べる
	for (int i = CHARACTER_PACMAN;i < CHARACTER_MAX;i++) {
		//調べる対象が自分自身か巣に帰る途中なら除外する
		if ((i == _characterNumber) || (characters[i].goHome)) continue;

		//進行方向の範囲内にキャラクター発見
		if ((characters[i].position.x >= characterWest) && (characters[i].position.x <= characterEast) &&
			(characters[i].position.y >= characterNorth) && (characters[i].position.y <= characterSouth))
		{
			//パックマンからモンスターと衝突
			if (_characterNumber == CHARACTER_PACMAN) crashBetweenCharacters(i);
			else {
				//モンスターからパックマンと衝突
				if (i == CHARACTER_PACMAN) crashBetweenCharacters(_characterNumber);

				//モンスター同士の衝突
				else {
					//進行方向の反対方向も調べる
					directionOfMovement = (directionOfMovement + 2) % DIRECTION_MAX;
					characters[_characterNumber].directionOfMovement = directionOfMovement;

					//モンスターの東西南北の範囲を初期化
					characterNorth = y;
					characterWest = x;
					characterSouth = y;
					characterEast = x;

					//衝突範囲を選定。範囲は現在の位置から反対方向へ、ブロックの幅(高さ)の分までの直線上
					for (int j = 0;j < DIRECTION_MAX;j++) {
						if (j == (directionOfMovement + 2) % DIRECTION_MAX) continue;

						if (directionOfMovement == DIRECTION_NORTH) characterNorth = y - BLOCK_HEIGHT;
						if (directionOfMovement == DIRECTION_WEST) characterWest = x - BLOCK_WIDTH;
						if (directionOfMovement == DIRECTION_SOUTH) characterSouth = y + BLOCK_HEIGHT;
						if (directionOfMovement == DIRECTION_EAST) characterEast = x + BLOCK_WIDTH;
					}

					//反対方向の範囲内にキャラクターがいるか調べる
					for (int k = CHARACTER_PACMAN;k < CHARACTER_MAX;k++) {
						//調べる対象が自分自身か、巣に帰る途中か、進行方向にいるなら除外する
						if ((k == _characterNumber) || (characters[k].goHome) || (k == i)) continue;

						//反対方向の範囲内にキャラクター発見
						if ((characters[k].position.x >= characterWest) && (characters[k].position.x <= characterEast) &&
							(characters[k].position.y >= characterNorth) && (characters[k].position.y <= characterSouth))
						{
							//モンスターとパックマンの衝突
							if (k == CHARACTER_PACMAN) crashBetweenCharacters(_characterNumber);
							else {
								//進行方向と反対方向の両方にモンスターがいればブロックの中央に戻る
								routeInit(_characterNumber);

								characters[_characterNumber].position.x = (x / BLOCK_WIDTH) * BLOCK_WIDTH + BLOCK_WIDTH / 2;
								characters[_characterNumber].position.y = (y / BLOCK_HEIGHT) * BLOCK_HEIGHT + BLOCK_HEIGHT / 2;
							}
						}
					}
				}
			}
		}
	}
}

//現在のブロックから東西南北4つのブロックの中で、最も目的地に近いブロックを探す処理
void goToDestination(int _characterNumber, VEC2 _destination) {
	/*
		4ブロックの目的地までの距離を1対1で比較して、一番近いブロックを勝ち残らせ続ける。
		勝ち残ったブロックの距離はtempDistanceに保存しておく。
		最後まで残ったブロックに向かって座標を進める。
	*/
	double tempDistance = pow(BLOCK_COLUMN_MAX, 2) + pow(BLOCK_ROW_MAX, 2);

	//進む予定の座標を渡す媒介変数
	int mediatorX = -1;
	int mediatorY = -1;

	for (int j = 0;j < DIRECTION_MAX;j++) {
		//進む予定の座標
		int x = characters[_characterNumber].position.x + directions[j][0];
		int y = characters[_characterNumber].position.y + directions[j][1];

		//進む予定の座標を一時的に保存
		int tempX = x;
		int tempY = y;

		//東西南北のブロックを取得
		switch (j) {
		case DIRECTION_NORTH:y -= BLOCK_HEIGHT / 2; break;
		case DIRECTION_SOUTH:y += BLOCK_HEIGHT / 2; break;//-1を消した
		case DIRECTION_WEST: x -= BLOCK_WIDTH / 2; break;
		case DIRECTION_EAST: x += BLOCK_WIDTH / 2; break;//-1を消した
		}

		/*
			座標をブロックの幅(高さ)で割ることでブロックの行列番号を取得する。
			座標が負になれば行列番号は-1を渡す。
		*/
		if (x < 0) {
			x = -1;
			y = y / BLOCK_HEIGHT;
		}
		else if (y < 0) {
			x = x / BLOCK_WIDTH;
			y = -1;
		}
		else {
			x = x / BLOCK_WIDTH;
			y = y / BLOCK_HEIGHT;
		}

		/*
			座標が迷路外に出ても割った余りから、補正できる。例えば、
			x = 21なら21 % BLOCK_COLUMN_MAX = 0、
			x = -1なら20 % BLOCK_COLUMN_MAX = 20、
		*/
		x = (x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
		y = (y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

		//_characterNumberの巣に帰るフラグが立っていれば処理しない
		int character = getCharacter(x, y);
		if ((!characters[_characterNumber].goHome) && (character >= CHARACTER_PACMAN)) {
			//パックマンがいれば衝突処理
			if (character == CHARACTER_PACMAN) {
				crashBetweenCharacters(_characterNumber);
				return;
			}
			//モンスターの巣に帰るフラグが立ってないなら除外
			else {
				if (!characters[character].goHome) continue;
			}
		}

		//壁があるか通過したブロックであれば、除外する
		else if ((blocks[y][x].type == BLOCK_TYPE_WALL) || (blocks[y][x].checked[_characterNumber])) continue;

		//進む予定の座標と目的地の距離を計算
		VEC2 distanceFromAToB = {
			_destination.x - x,
			_destination.y - y
		};
		double distance = pow(distanceFromAToB.x, 2) + pow(distanceFromAToB.y, 2);

		//4ブロックを比較して短ければmeditorに渡す
		if (distance < tempDistance) {
			tempDistance = distance;
			mediatorX = tempX;
			mediatorY = tempY;
		}
	}

	//進めるブロックがないなら、ルートを初期化する
	if ((mediatorX == -1) && (mediatorY == -1)) routeInit(_characterNumber);
	//座標を確定して、進んだブロックにはフラグを立てる
	else {
		characters[_characterNumber].position.x = mediatorX;
		characters[_characterNumber].position.y = mediatorY;

		mediatorX = mediatorX / BLOCK_WIDTH;
		mediatorY = mediatorY / BLOCK_HEIGHT;
		blocks[mediatorY][mediatorX].checked[_characterNumber] = true;
	}
}

//パックマンに撃退されたイジケモンスターが、巣の前に到着した際の処理
void goHome(int _characterNumber) {
	//目的地に着いた時の処理
	if ((characters[_characterNumber].position.x == inFrontOfNestPointX) &&
		(characters[_characterNumber].position.y == inFrontOfNestPointY))
	{
		//巣待機時での初期状態に戻す
		characters[_characterNumber].position.x = 10 * BLOCK_WIDTH + BLOCK_WIDTH / 2;
		characters[_characterNumber].position.y = 12 * BLOCK_HEIGHT + BLOCK_HEIGHT / 2;
		characters[_characterNumber].monsterSpeed = MONSTER_SPEED;
		characters[_characterNumber].invincible = true;
		characters[_characterNumber].stayHome = true;
		characters[_characterNumber].goHome = false;

		//巣に待機できるのは3匹で、4匹になるなら1匹外に出す
		int count = 0;
		for (int j = CHARACTER_RED;j < CHARACTER_MAX;j++)
			if (characters[j].stayHome) count++;
		if (count >= 4) {
			for (int j = CHARACTER_RED;j < CHARACTER_MAX;j++) {
				if (j == _characterNumber) continue;
				else {
					if (characters[j].stayHome) {
						characters[j].position.x = inFrontOfNestPointX;
						characters[j].position.y = inFrontOfNestPointY;

						characters[j].stayHome = false;
						break;
					}
				}
			}
		}
	}
}

//ブロックやキャラクターを描画する関数
void Display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//迷路の描画	
	for (int y = 0;y < BLOCK_ROW_MAX;y++)
		for (int x = 0;x < BLOCK_COLUMN_MAX;x++) {
			if ((blocks[y][x].type == BLOCK_TYPE_WALL) || (blocks[y][x].type == BLOCK_TYPE_NONE)) {
				if ((y == 11) && (x == 10))
					glColor3ub(0xff, 0xff, 0xff);
				else {
					if (blocks[y][x].type == BLOCK_TYPE_WALL)
						glColor3ub(0x00, 0x00, 0xff);
					if (blocks[y][x].type == BLOCK_TYPE_NONE)
						glColor3ub(0x00, 0x00, 0x00);
				}

				glPushMatrix();
				{
					if ((y == 11) && (x == 10))
						glTranslatef((GLfloat)x * BLOCK_WIDTH, (GLfloat)y * BLOCK_HEIGHT + BLOCK_HEIGHT / 2, 0);
					else
						glTranslatef((GLfloat)x * BLOCK_WIDTH, (GLfloat)y * BLOCK_HEIGHT, 0);
					glBegin(GL_QUADS);
					{
						glVertex2f(0, 0);
						glVertex2f(BLOCK_WIDTH, 0);

						if ((y == 11) && (x == 10)) {
							glVertex2f(BLOCK_WIDTH, BLOCK_HEIGHT / 2);
							glVertex2f(0, BLOCK_HEIGHT / 2);
						}
						else {
							glVertex2f(BLOCK_WIDTH, BLOCK_HEIGHT);
							glVertex2f(0, BLOCK_HEIGHT);
						}
					}
					glEnd();
				}
				glPopMatrix();
			}

			if ((blocks[y][x].type == BLOCK_TYPE_DOT) || (blocks[y][x].type == BLOCK_TYPE_POWER)) {

				glColor3ub(0xff, 0xff, 0x00);

				glPushMatrix();
				{
					glTranslatef(
						(GLfloat)x * BLOCK_WIDTH + BLOCK_WIDTH / 2,
						(GLfloat)y * BLOCK_HEIGHT + BLOCK_HEIGHT / 2,
						0);

					if (blocks[y][x].type == BLOCK_TYPE_DOT)
						glScalef(2, 2, 0);
					if (blocks[y][x].type == BLOCK_TYPE_POWER)
						glScalef(8, 8, 0);

					glBegin(GL_TRIANGLE_FAN);
					{
						glVertex2f(0, 0);
						int n = 32;
						for (int i = 0;i <= n;i++) {
							float r = glm::pi<float>() * 2 * i / n;
							glVertex2f(cosf(r), -sinf(r));
						}
					}
					glEnd();
				}
				glPopMatrix();
			}
		}

	//キャラクターの描画
	for (int j = CHARACTER_PACMAN;j < CHARACTER_MAX;j++) {

		int x = characters[j].position.x;
		int y = characters[j].position.y;

		//パックマン
		if (j == CHARACTER_PACMAN) {
			glColor3ub(0xff, 0xff, 0x00);

			//体と口
			glPushMatrix();
			{
				glTranslatef(
					(GLfloat)x,
					(GLfloat)y,
					0);

				glBegin(GL_TRIANGLE_FAN);
				{
					int closeAMouse = 0;
					if (characters[CHARACTER_PACMAN].moveAnimation)
						closeAMouse = 0;
					else closeAMouse = 3;

					int directionOfMovement = -1;
					switch (characters[CHARACTER_PACMAN].directionOfMovement) {
					case DIRECTION_NORTH: directionOfMovement = 8; break;
					case DIRECTION_WEST: directionOfMovement = 16; break;
					case DIRECTION_SOUTH: directionOfMovement = 24; break;
					case DIRECTION_EAST: directionOfMovement = 32; break;
					default:
						directionOfMovement = 0;
						closeAMouse = 0;
						break;
					}

					glVertex2f(0, 0);

					GLfloat radius = BLOCK_WIDTH / 2;

					int n = 32;
					for (int i = directionOfMovement + closeAMouse;i <= directionOfMovement + n - closeAMouse;i++) {
						float r = glm::pi<float>() * 2 * i / n;
						glVertex2f(radius * cosf(r), -radius * sinf(r));
					}
				}
				glEnd();
			}
			glPopMatrix();
		}
		//モンスター
		else {
			enum {
				LEFT_EYE,
				RIGHT_EYE
			};

			enum {
				WHITE_EYE,
				BLUE_EYE
			};

			if (characters[j].invincible) {
				if (j == CHARACTER_RED)
					glColor3ub(0xff, 0x00, 0x00);
				if (j == CHARACTER_BLUE)
					glColor3ub(0x00, 0xff, 0xff);
				if (j == CHARACTER_PINK)
					glColor3ub(0xff, 0x69, 0xb4);
				if (j == CHARACTER_ORANGE)
					glColor3ub(0xff, 0x8c, 0x00);
			}
			else glColor3ub(0x00, 0x00, 0xff);

			//体
			if (!characters[j].goHome) {
				GLfloat characterDiameter = BLOCK_WIDTH + 4;
				glPushMatrix();
				{
					glTranslatef(
						(GLfloat)x,
						(GLfloat)y,
						0);

					glBegin(GL_TRIANGLE_FAN);
					{
						glVertex2f(0, 0);

						GLfloat radius = characterDiameter / 2;

						int n = 32;
						for (int i = 0;i <= n / 2;i++) {
							float r = glm::pi<float>() * 2 * i / n;
							glVertex2f(radius * cosf(r), -radius * sinf(r));
						}
					}
					glEnd();
				}
				glPopMatrix();

				glPushMatrix();
				{
					glBegin(GL_TRIANGLE_FAN);
					{
						glVertex2f((GLfloat)x + characterDiameter / 2, (GLfloat)y);
						glVertex2f((GLfloat)x + characterDiameter / 2, (GLfloat)y + characterDiameter / 2);
						glVertex2f((GLfloat)x + characterDiameter / 2 - characterDiameter * 1 / 4, (GLfloat)y + characterDiameter * 2 / 6);
						glVertex2f((GLfloat)x + characterDiameter / 2 - characterDiameter * 1 / 3, (GLfloat)y + characterDiameter / 2);
						glVertex2f((GLfloat)x + characterDiameter / 2 - characterDiameter * 2 / 4, (GLfloat)y + characterDiameter * 2 / 6);//下の中心
						glVertex2f((GLfloat)x + characterDiameter / 2 - characterDiameter * 2 / 3, (GLfloat)y + characterDiameter / 2);
						glVertex2f((GLfloat)x + characterDiameter / 2 - characterDiameter * 3 / 4, (GLfloat)y + characterDiameter * 2 / 6);
						glVertex2f((GLfloat)x - characterDiameter / 2, (GLfloat)y + characterDiameter / 2);
						glVertex2f((GLfloat)x - characterDiameter / 2, (GLfloat)y);
					}
					glEnd();
				}
				glPopMatrix();
			}

			//目
			for (int k = LEFT_EYE;k <= RIGHT_EYE;k++) {
				for (int l = WHITE_EYE;l <= BLUE_EYE;l++) {
					if ((l == WHITE_EYE) && (!characters[j].invincible) && (!characters[j].goHome))
						continue;

					glPushMatrix();
					{
						if (characters[j].invincible) {
							if (l == WHITE_EYE) glColor3ub(0xff, 0xff, 0xff);
							if (l == BLUE_EYE) glColor3ub(0x00, 0x00, 0xff);
						}
						else {
							if (characters[j].goHome) {
								if (l == WHITE_EYE) glColor3ub(0xff, 0xff, 0xff);
								if (l == BLUE_EYE) glColor3ub(0x00, 0x00, 0xff);
							}
							else glColor3ub(0xff, 0xff, 0xff);
						}

						GLfloat blueEyePosutionX = 0;
						GLfloat blueEyePosutionY = 0;

						if (l == BLUE_EYE) {
							switch (characters[j].directionOfMovement) {
							case DIRECTION_NORTH: blueEyePosutionY = -BLOCK_WIDTH / 8; break;
							case DIRECTION_WEST: blueEyePosutionX = -BLOCK_WIDTH / 8; break;
							case DIRECTION_SOUTH: blueEyePosutionY = BLOCK_WIDTH / 8; break;
							case DIRECTION_EAST: blueEyePosutionX = BLOCK_WIDTH / 8; break;
							}
						}

						if (k == LEFT_EYE)
							glTranslatef(
								(GLfloat)x - BLOCK_WIDTH * 1 / 3 + blueEyePosutionX,
								(GLfloat)y + blueEyePosutionY,
								0);

						if (k == RIGHT_EYE)
							glTranslatef(
								(GLfloat)x + BLOCK_WIDTH * 1 / 3 + blueEyePosutionX,
								(GLfloat)y + blueEyePosutionY,
								0);

						glBegin(GL_TRIANGLE_FAN);
						{
							GLfloat radius = 0;

							if (l == WHITE_EYE) radius = BLOCK_WIDTH / 4;
							if (l == BLUE_EYE) radius = BLOCK_WIDTH / 8;

							glVertex2f(0, 0);

							int n = 32;
							for (int i = 0;i <= n;i++) {
								float r = glm::pi<float>() * 2 * i / n;
								glVertex2f(radius * cosf(r), -radius * sinf(r));
							}
						}
						glEnd();
					}
					glPopMatrix();
				}
			}

		}
	}

	glutSwapBuffers();
}

//キャラクターの動きを処理するメインの関数
void Idle(void) {
	if (timePasses(CLOCK_UPDATE)) {
		clockMode[CLOCK_UPDATE].lastClock = clock();

		//ゲーム全体の初期化
		if (flag.init) Init();

		//時間制限
		for (int j = CLOCK_GO_OUT;j <= CLOCK_TERRITORY_MODE;j++) {
			if (timePasses(j)) {
				clockMode[j].lastClock = clock();

				if (j == CLOCK_GO_OUT) {
					for (int i = CHARACTER_RED;i < CHARACTER_MAX;i++) {
						if (characters[i].stayHome) {
							characters[i].position = {
								inFrontOfNestPointX,
								inFrontOfNestPointY
							};
							characters[i].stayHome = false;
							break;
						}
					}
				}
				if (j == CLOCK_INVINCIBLE) {
					for (int i = CHARACTER_RED;i < CHARACTER_MAX;i++) {
						if (characters[i].goHome) continue;
						else characters[i].invincible = true;
					}
					flag.invincibleMode = false;
				}
				if (j == CLOCK_CHASE_MODE) {
					flag.territoryMode = true;
					flag.chaseMode = false;
					clockMode[j].lastClock = clock();
				}
				if (j == CLOCK_TERRITORY_MODE) {
					flag.chaseMode = true;
					flag.territoryMode = false;
					clockMode[j].lastClock = clock();
				}
			}
		}

		//キャラクター
		for (int i = CHARACTER_PACMAN;i < CHARACTER_MAX;i++) {
			//パックマンの処理
			if (i == CHARACTER_PACMAN) {
				//新しい座標に進むために古い座標を保存
				characters[i].lastPosition = characters[i].position;

				//ブロックの中心にいれば進路変更できる
				if (isOnThePoint(i)) {
					if (keysPressed['w']) characters[i].directionOfMovement = DIRECTION_NORTH;
					if (keysPressed['s']) characters[i].directionOfMovement = DIRECTION_SOUTH;
					if (keysPressed['a']) characters[i].directionOfMovement = DIRECTION_WEST;
					if (keysPressed['d']) characters[i].directionOfMovement = DIRECTION_EAST;

					int x = characters[i].position.x + directions[characters[i].directionOfMovement][0];
					int y = characters[i].position.y + directions[characters[i].directionOfMovement][1];

					int tempX = x;
					int tempY = y;

					//指定した方角のブロックに移動
					switch (characters[i].directionOfMovement) {
					case DIRECTION_NORTH:y -= BLOCK_HEIGHT / 2; break;
					case DIRECTION_SOUTH:y += BLOCK_HEIGHT / 2; break;
					case DIRECTION_WEST: x -= BLOCK_WIDTH / 2; break;
					case DIRECTION_EAST: x += BLOCK_WIDTH / 2; break;
					}

					x = x / BLOCK_WIDTH;
					y = y / BLOCK_HEIGHT;

					x = (x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
					y = (y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

					//ブロックが壁じゃない
					if (blocks[y][x].type != BLOCK_TYPE_WALL) {
						//ブロックにモンスターがいれば衝突処理
						int character = getCharacter(x, y);
						if (character > CHARACTER_PACMAN)
							crashBetweenCharacters(character);

						/*
							ブロックにパワーエサがあればモンスターがイジケて、
							追跡モードから縄張りモードに変わる
						*/
						if (blocks[y][x].type == BLOCK_TYPE_POWER) {
							for (int j = CHARACTER_RED;j < CHARACTER_MAX;j++)
								characters[j].invincible = false;

							flag.invincibleMode = true;
							clockMode[CLOCK_INVINCIBLE].lastClock = clock();

							flag.territoryMode = true;
							flag.chaseMode = false;
							clockMode[CLOCK_TERRITORY_MODE].lastClock = clock();
						}

						/*
							ブロックにパワーエサかドットがあればブロックから
							それらの描画を消して、カウントする
						*/
						if (blocks[y][x].type != BLOCK_TYPE_NONE) {
							blocks[y][x].type = BLOCK_TYPE_NONE;
							characters[i].dotAndPowerCount--;
						}

						//座標の確定
						characters[i].position.x = tempX;
						characters[i].position.y = tempY;
					}
					//壁があれば現在の座標で止まる
					else characters[i].directionOfMovement = -1;

					//決めた方向に進んでいる場合
					if (characters[i].directionOfMovement != -1) {
						//描画の際、trueなら口を開ける、falseなら口を閉じる
						if (characters[i].moveAnimation) characters[i].moveAnimation = false;
						else characters[i].moveAnimation = true;
					}
				}

				//迷路内からパワーエサとドットがなくなれば初期化フラグを立てる
				if (characters[i].dotAndPowerCount <= 0) flag.init = true;
			}
			//モンスターの処理
			else {
				//巣に帰るフラグが立っている場合
				if (characters[i].goHome) {
					//新しい座標に進むために古い座標を保存
					characters[i].lastPosition = characters[i].position;

					//目的地である巣の前の行列番号
					VEC2 destination = {
						inFrontOfNestPointX / BLOCK_WIDTH,
						inFrontOfNestPointY / BLOCK_HEIGHT
					};

					//ブロック中央に着いたら進路変更
					if (isOnThePoint(i)) {
						goToDestination(i, destination);
						getDirectionOfMovement(i);
					}
				}
				else {
					//縄張りモード
					if (flag.territoryMode) {
						//新しい座標に進むために古い座標を保存
						characters[i].lastPosition = characters[i].position;

						//ブロック中央に着いたら進路変更
						if (isOnThePoint(i)) {
							goToDestination(i, territoryPosition[i]);
							getDirectionOfMovement(i);
						}
					}

					//追跡モード
					if (flag.chaseMode) {
						if (i == CHARACTER_RED) {
							//赤色の目的地はパックマンのブロック

							//新しい座標に進むために古い座標を保存
							characters[i].lastPosition = characters[i].position;

							//パックマンのいるブロック
							VEC2 destination = {
								characters[CHARACTER_PACMAN].position.x / BLOCK_WIDTH,
								characters[CHARACTER_PACMAN].position.y / BLOCK_HEIGHT
							};

							//ブロック中央に着いたら進路変更
							if (isOnThePoint(i)) {
								goToDestination(i, destination);
								getDirectionOfMovement(i);
							}

						}

						if (i == CHARACTER_BLUE) {
							//水色の目的地はパックマンを中心にして赤色と点対称になるブロック

							//新しい座標に進むために古い座標を保存
							characters[i].lastPosition = characters[i].position;

							//ブロック中央に着いたら進路変更
							if (isOnThePoint(i)) {
								//パックマンのいるブロック
								int pacmanX = characters[CHARACTER_PACMAN].position.x / BLOCK_WIDTH;
								int pacmanY = characters[CHARACTER_PACMAN].position.y / BLOCK_HEIGHT;

								//赤色のいるブロック
								int redX = characters[CHARACTER_RED].position.x / BLOCK_WIDTH;
								int redY = characters[CHARACTER_RED].position.y / BLOCK_HEIGHT;

								//赤色に対するパックマンの相対座標
								VEC2 distanceFromAToB = {
									pacmanX - redX,
									pacmanY - redY,
								};

								//該当の点対称になる位置
								int x = pacmanX + distanceFromAToB.x;
								int y = pacmanY + distanceFromAToB.y;

								//迷路外に出れば真反対の位置に戻る
								x = (x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
								y = (y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

								VEC2 destination = { x,y };

								goToDestination(i, destination);
								getDirectionOfMovement(i);
							}
						}

						if (i == CHARACTER_PINK) {
							//ピンク色の目的地はパックマンから少し前のブロック

							//新しい座標に進むために古い座標を保存
							characters[i].lastPosition = characters[i].position;

							//ブロック中央に着いたら進路変更
							if (isOnThePoint(i)) {
								VEC2 destination = { -1,-1 };

								//パックマンのいるブロック
								int pacmanX = characters[CHARACTER_PACMAN].position.x / BLOCK_WIDTH;
								int pacmanY = characters[CHARACTER_PACMAN].position.y / BLOCK_HEIGHT;

								//少し前のブロックは3つ先
								const int frontOfPacmanPos = 3;//6

								//パックマンの進行方向に応じて目的地を選ぶ
								switch (characters[CHARACTER_PACMAN].directionOfMovement) {
								case DIRECTION_NORTH:
									destination.y = pacmanY - frontOfPacmanPos;
									destination.x = pacmanX;
									break;
								case DIRECTION_SOUTH:
									destination.y = pacmanY + frontOfPacmanPos;
									destination.x = pacmanX;
									break;
								case DIRECTION_WEST:
									destination.x = pacmanX - frontOfPacmanPos;
									destination.y = pacmanY;
									break;
								case DIRECTION_EAST:
									destination.x = pacmanX + frontOfPacmanPos;
									destination.y = pacmanY;
									break;
								default:
									//DIRECTION_NORTHと同じ
									destination.y = pacmanY - frontOfPacmanPos;
									destination.x = pacmanX;
									break;
								}

								//迷路外に出れば真反対の位置に戻る
								destination.x = (destination.x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
								destination.y = (destination.y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

								goToDestination(i, destination);
								getDirectionOfMovement(i);
							}
						}

						if (i == CHARACTER_ORANGE) {
							/*
								オレンジ色の目的地は、パックマンとオレンジ色の距離を比較して、
								遠ければパックマンのブロック、近ければパックマンから遠くのブロック
							*/

							//新しい座標に進むために古い座標を保存
							characters[i].lastPosition = characters[i].position;

							//ブロック中央に着いたら進路変更
							if (isOnThePoint(i)) {
								//追跡するか逃げるかの基準となる距離
								const int chaseOrEscapeDistance = 3;

								//パックマンのいるブロック
								int pacmanX = characters[CHARACTER_PACMAN].position.x / BLOCK_WIDTH;
								int pacmanY = characters[CHARACTER_PACMAN].position.y / BLOCK_HEIGHT;

								//オレンジ色のいるブロック
								int orangeX = characters[i].position.x / BLOCK_WIDTH;
								int orangeY = characters[i].position.y / BLOCK_HEIGHT;

								//オレンジ色に対するパックマンの相対座標
								double distanceFromAToB = pow(pacmanX - orangeX, 2) + pow(pacmanY - orangeY, 2);

								//オレンジ色がパックマンと近ければ
								if (distanceFromAToB < chaseOrEscapeDistance) {
									/*
										現在のオレンジ色のブロックから、東西南北のブロックで、
										最もパックマンから遠いブロックを目的地とする
									*/

									VEC2 destination = { -1,-1 };
									double tempDistance = pow(0, 2) + pow(0, 2);

									for (int j = 0;j < DIRECTION_MAX;j++) {
										int x = orangeX + directions[j][0];
										int y = orangeY + directions[j][1];

										int relativeX = x - pacmanX;
										int relativeY = y - pacmanY;

										double distance = pow(relativeX, 2) + pow(relativeY, 2);

										if (distance > tempDistance) {
											destination.x = x;
											destination.y = y;
										}
									}

									//迷路外に出れば真反対の位置に戻る
									destination.x = (destination.x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
									destination.y = (destination.y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

									goToDestination(i, destination);
								}
								//オレンジ色がパックマンと遠ければ
								if (distanceFromAToB >= chaseOrEscapeDistance) {
									VEC2 pacmanR = { pacmanX, pacmanY };
									goToDestination(i, pacmanR);
								}

								getDirectionOfMovement(i);
							}

						}
					}
				}
			}
			//座標を進める
			if (characters[i].directionOfMovement != -1) {
				for (int count = 0;count < characters[i].monsterSpeed;count++) {
					//新しい座標に進むために古い座標を保存
					characters[i].lastPosition = characters[i].position;

					//フラグが立ってれば、巣の前に着いたかのチェック
					if (characters[i].goHome) goHome(i);

					//ブロック中央に着いたら進路変更
					if (isOnThePoint(i)) break;
					else {
						//キャラクター同士で衝突しそうなら進路変更
						changeDirectionOfMovement(i);

						//迷路外に出れば真反対の位置に戻る
						overflowFromMap(i);

						//座標を進める
						characters[i].position.x = characters[i].position.x + directions[characters[i].directionOfMovement][0];
						characters[i].position.y = characters[i].position.y + directions[characters[i].directionOfMovement][1];
					}
				}
			}
		}

		glutPostRedisplay();
	}
}

//キーボードが押されてるならtrue
void Keyboard(unsigned char _key, int, int) {
	keysPressed[_key] = true;
}

//キーボードが押されてないならfalse
void KeyboardUp(unsigned char _key, int, int) {
	keysPressed[_key] = false;
}

int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GL_DOUBLE);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("PACMAN");
	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyboardUp);
	Init();
	glutMainLoop();
}