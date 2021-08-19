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

//�u���b�N�̎��
enum {
	BLOCK_TYPE_WALL,
	BLOCK_TYPE_DOT,
	BLOCK_TYPE_POWER,
	BLOCK_TYPE_NONE,
	BLOCK_TYPE_NAX
};

//�L�����N�^�[�̎��
enum {
	CHARACTER_PACMAN,
	CHARACTER_RED,
	CHARACTER_BLUE,
	CHARACTER_PINK,
	CHARACTER_ORANGE,
	CHARACTER_MAX
};

//���Ԃ̎��
enum {
	CLOCK_GO_OUT,
	CLOCK_INVINCIBLE,
	CLOCK_CHASE_MODE,
	CLOCK_TERRITORY_MODE,
	CLOCK_UPDATE,
	CLOCK_MAX
};

//���p
enum {
	DIRECTION_NORTH,
	DIRECTION_WEST,
	DIRECTION_SOUTH,
	DIRECTION_EAST,
	DIRECTION_MAX
};

//���p���Ƃ̐i�ދ���
int directions[][2] = {
	{0, -1},	//DIRECTION_NORTH,
	{-1, 0},	//DIRECTION_WEST,
	{0, 1},		//DIRECTION_SOUTH,
	{1, 0},		//DIRECTION_EAST,
};

//���H���̃u���b�N��27�~21����A1�u���b�N�̕��ƍ�����26
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

//2�����x�N�g��
typedef struct {
	int x, y;
}VEC2;

//�L�����N�^�[���Ƃ̓꒣��n�_
VEC2 territoryPosition[CHARACTER_MAX] = {
	{-1, -1},	//CHARACTER_PACMAN,
	{19, 1},	//CHARACTER_RED,
	{19, 25},	//CHARACTER_BLUE,
	{1, 1},		//CHARACTER_PINK,
	{1, 25},	//CHARACTER_ORANGE,
};

//�t���O
typedef struct {
	bool init;			//�v���O����������
	bool invincibleMode;//�p�b�N�}�����G
	bool territoryMode;	//�꒣�胂�[�h
	bool chaseMode;		//�ǐՃ��[�h
}FLAG;

FLAG flag;

//���Ԑ���
typedef struct {
	clock_t lastClock;	//�Ō�̌v������
	clock_t nowClock;	//�ŐV�̌v������
	float interval;		//���ԊԊu
}CLOCK;

CLOCK clockMode[CLOCK_MAX];

//�u���b�N
typedef struct {
	int type;					//�u���b�N�̎��
	bool checked[CHARACTER_MAX];//�L�����N�^�[���Ƃ̈ړ������t���O
}BLOCK;

BLOCK blocks[BLOCK_ROW_MAX][BLOCK_COLUMN_MAX];

//�L�����N�^�[
typedef struct {
	VEC2 position;			//���W
	VEC2 lastPosition;		//�Â����W
	bool invincible;		//���G�t���O
	bool stayHome;			//���ҋ@�t���O�@�����X�^�[�̂�
	bool goHome;			//���A��t���O�@�����X�^�[�̂�
	bool moveAnimation;		//���̊J�@�p�b�N�}���̂�
	int directionOfMovement;//�i�s����
	int monsterSpeed;		//����
	int dotAndPowerCount;	//�J�E���g�@�p�b�N�}���̂�
}CHARACTER;

CHARACTER characters[CHARACTER_MAX];

//�����X�^�[���ҋ@���鑃�̑O�̍��W
int inFrontOfNestPointX;
int inFrontOfNestPointY;

//�L�[�{�[�h256���̃o�C�g�z��
bool keysPressed[256];

//�ړI�n�Ɍ������r���łǂ��ɂ��i�߂Ȃ��Ȃ�΁A���[�g�����������鏈��
void routeInit(int _characterNumber) {
	for (int y = 0;y < BLOCK_ROW_MAX;y++)
		for (int x = 0;x < BLOCK_COLUMN_MAX;x++)
			if (blocks[y][x].checked[_characterNumber])
				blocks[y][x].checked[_characterNumber] = false;

	characters[_characterNumber].directionOfMovement = -1;
}

//�v���O�����J�n����ɏ��������鏈��
void Init(void) {
	//������mapData�������u���b�N�̎�ނ𕪂���A���̍ۃL�����N�^�[�̏����ʒu���擾����
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

	//���H�����̓�����O�̍��W
	inFrontOfNestPointX = inFrontOfNestPointX * BLOCK_WIDTH + BLOCK_WIDTH / 2;
	inFrontOfNestPointY = inFrontOfNestPointY * BLOCK_HEIGHT + BLOCK_HEIGHT / 2;

	//�꒣�胂�[�h����J�n����A����ȊO�̓t���O�����낷
	flag.init = false;
	flag.territoryMode = true;
	flag.chaseMode = false;
	flag.invincibleMode = false;

	//�L�����N�^�[�̏�Ԃ�������
	for (int i = CHARACTER_PACMAN;i < CHARACTER_MAX;i++) {
		if (i == CHARACTER_PACMAN) {
			characters[i].invincible = false;
			characters[i].stayHome = false;
			characters[i].monsterSpeed = PACMAN_SPEED;

			//���H���̃h�b�g�ƃp���[�G�T�𐔂���
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

		//�L�����N�^�[�̏������W
		characters[i].position.x = characters[i].position.x * BLOCK_WIDTH + BLOCK_WIDTH / 2;
		characters[i].position.y = characters[i].position.y * BLOCK_HEIGHT + BLOCK_HEIGHT / 2;

		characters[i].moveAnimation = false;
		characters[i].goHome = false;
		characters[i].directionOfMovement = -1;
		routeInit(i);
	}

	//���ԊԊu�̏�����
	clockMode[CLOCK_GO_OUT].interval = 5000;
	clockMode[CLOCK_INVINCIBLE].interval = 10000;
	clockMode[CLOCK_CHASE_MODE].interval = 20000;
	clockMode[CLOCK_TERRITORY_MODE].interval = 10000;
	clockMode[CLOCK_UPDATE].interval = 1000 / 60;

	//�����X�^�[�́A������̒E�o�����Ɠ꒣�胂�[�h�̊J�n�������擾����
	clockMode[CLOCK_GO_OUT].lastClock = clock();
	clockMode[CLOCK_TERRITORY_MODE].lastClock = clock();
}

//�ǐՃ��[�h��꒣�胂�[�h�ȂǁA���Ԍo�߂��Ǘ����鏈��
bool timePasses(int _clockNumber) {
	//���݂̎���(_nowClock)���Ō�ɑ���������(_lastClock)����_interval�b�o�߂��Ă����true��n��
	clockMode[_clockNumber].nowClock = clock();
	if (clockMode[_clockNumber].nowClock >= clockMode[_clockNumber].lastClock + clockMode[_clockNumber].interval)
		return true;
	else return false;
}

//�u���b�N�̒����ɂ��邩�̔��菈���Atrue�Ȃ�i�H�ύX�ł���
bool isOnThePoint(int _characterNumber) {
	int x = characters[_characterNumber].position.x / BLOCK_WIDTH;
	int y = characters[_characterNumber].position.y / BLOCK_HEIGHT;

	if ((characters[_characterNumber].position.x == x * BLOCK_WIDTH + BLOCK_WIDTH / 2) &&
		(characters[_characterNumber].position.y == y * BLOCK_HEIGHT + BLOCK_HEIGHT / 2))
		return true;

	else return false;
}

//���E�̒ʘH��ʉ߂����ۂ̃��[�v����
void overflowFromMap(int _characterNumber) {
	if (characters[_characterNumber].position.x > WINDOW_WIDTH + BLOCK_WIDTH / 2)
		characters[_characterNumber].position.x = -BLOCK_WIDTH / 2;

	else if (characters[_characterNumber].position.x < -BLOCK_WIDTH / 2)
		characters[_characterNumber].position.x = WINDOW_WIDTH + BLOCK_WIDTH / 2;
}

//�i�񂾃u���b�N�ɃL�����N�^�[������΁A���̔ԍ����擾�ł���
int getCharacter(int _x, int _y) {
	for (int i = CHARACTER_PACMAN;i < CHARACTER_MAX;i++) {

		int characterPosX = characters[i].position.x / BLOCK_WIDTH;
		int characterPosY = characters[i].position.y / BLOCK_HEIGHT;

		if ((_x == characterPosX) && (_y == characterPosY))
			return i;
	}
	return -1;
}

//���݂̍��W�ƑO��̍��W�����ƂɁA�i�s�������擾����
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

//�p�b�N�}���ƃ����X�^�[���Փ˂����ۂ̏���
void crashBetweenCharacters(int _characterNumber) {
	/*
		�����X�^�[���C�W�P�Ă�Ȃ�A�u���b�N�̒��S�Ɉړ����Ă��烋�[�g�������������ɋA��t���O�𗧂ĂāA
		�ʏ��3�{�̑����ő��ɋA��B���̍ۂ̓L�����N�^�[���m���Փ˂��Ă����蔲����B
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
	//�����X�^�[���C�W�P�ĂȂ��Ȃ�A�v���O�����S�̂̏������t���O�𗧂Ă�
	else flag.init = true;
}

//�����X�^�[���ړ����ɖ������m�ŏՓ˂���Ȃ�A�i�H�ύX���鏈��
void changeDirectionOfMovement(int _characterNumber) {
	//�����X�^�[�����ɖ߂�r���Ȃ珈�����Ȃ��B�܂�Փ˂��Ă����蔲����
	if (characters[_characterNumber].goHome) return;

	int x = characters[_characterNumber].position.x;
	int y = characters[_characterNumber].position.y;

	int directionOfMovement = characters[_characterNumber].directionOfMovement;

	//�����X�^�[�̓�����k�͈̔͂�������
	int characterNorth = y;
	int characterWest = x;
	int characterSouth = y;
	int characterEast = x;

	//�Փ˔͈͂�I��B�͈͂͌��݂̈ʒu����i�s�����ցA�u���b�N�̕�(����)�̕��܂ł̒�����
	for (int j = 0;j < DIRECTION_MAX;j++) {
		if (j == (directionOfMovement + 2) % DIRECTION_MAX) continue;

		if (directionOfMovement == DIRECTION_NORTH) characterNorth = y - BLOCK_HEIGHT;
		if (directionOfMovement == DIRECTION_WEST) characterWest = x - BLOCK_WIDTH;
		if (directionOfMovement == DIRECTION_SOUTH) characterSouth = y + BLOCK_HEIGHT;
		if (directionOfMovement == DIRECTION_EAST) characterEast = x + BLOCK_WIDTH;
	}

	//�i�s�����͈͓̔��ɃL�����N�^�[�����邩���ׂ�
	for (int i = CHARACTER_PACMAN;i < CHARACTER_MAX;i++) {
		//���ׂ�Ώۂ��������g�����ɋA��r���Ȃ珜�O����
		if ((i == _characterNumber) || (characters[i].goHome)) continue;

		//�i�s�����͈͓̔��ɃL�����N�^�[����
		if ((characters[i].position.x >= characterWest) && (characters[i].position.x <= characterEast) &&
			(characters[i].position.y >= characterNorth) && (characters[i].position.y <= characterSouth))
		{
			//�p�b�N�}�����烂���X�^�[�ƏՓ�
			if (_characterNumber == CHARACTER_PACMAN) crashBetweenCharacters(i);
			else {
				//�����X�^�[����p�b�N�}���ƏՓ�
				if (i == CHARACTER_PACMAN) crashBetweenCharacters(_characterNumber);

				//�����X�^�[���m�̏Փ�
				else {
					//�i�s�����̔��Ε��������ׂ�
					directionOfMovement = (directionOfMovement + 2) % DIRECTION_MAX;
					characters[_characterNumber].directionOfMovement = directionOfMovement;

					//�����X�^�[�̓�����k�͈̔͂�������
					characterNorth = y;
					characterWest = x;
					characterSouth = y;
					characterEast = x;

					//�Փ˔͈͂�I��B�͈͂͌��݂̈ʒu���甽�Ε����ցA�u���b�N�̕�(����)�̕��܂ł̒�����
					for (int j = 0;j < DIRECTION_MAX;j++) {
						if (j == (directionOfMovement + 2) % DIRECTION_MAX) continue;

						if (directionOfMovement == DIRECTION_NORTH) characterNorth = y - BLOCK_HEIGHT;
						if (directionOfMovement == DIRECTION_WEST) characterWest = x - BLOCK_WIDTH;
						if (directionOfMovement == DIRECTION_SOUTH) characterSouth = y + BLOCK_HEIGHT;
						if (directionOfMovement == DIRECTION_EAST) characterEast = x + BLOCK_WIDTH;
					}

					//���Ε����͈͓̔��ɃL�����N�^�[�����邩���ׂ�
					for (int k = CHARACTER_PACMAN;k < CHARACTER_MAX;k++) {
						//���ׂ�Ώۂ��������g���A���ɋA��r�����A�i�s�����ɂ���Ȃ珜�O����
						if ((k == _characterNumber) || (characters[k].goHome) || (k == i)) continue;

						//���Ε����͈͓̔��ɃL�����N�^�[����
						if ((characters[k].position.x >= characterWest) && (characters[k].position.x <= characterEast) &&
							(characters[k].position.y >= characterNorth) && (characters[k].position.y <= characterSouth))
						{
							//�����X�^�[�ƃp�b�N�}���̏Փ�
							if (k == CHARACTER_PACMAN) crashBetweenCharacters(_characterNumber);
							else {
								//�i�s�����Ɣ��Ε����̗����Ƀ����X�^�[������΃u���b�N�̒����ɖ߂�
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

//���݂̃u���b�N���瓌����k4�̃u���b�N�̒��ŁA�ł��ړI�n�ɋ߂��u���b�N��T������
void goToDestination(int _characterNumber, VEC2 _destination) {
	/*
		4�u���b�N�̖ړI�n�܂ł̋�����1��1�Ŕ�r���āA��ԋ߂��u���b�N�������c�点������B
		�����c�����u���b�N�̋�����tempDistance�ɕۑ����Ă����B
		�Ō�܂Ŏc�����u���b�N�Ɍ������č��W��i�߂�B
	*/
	double tempDistance = pow(BLOCK_COLUMN_MAX, 2) + pow(BLOCK_ROW_MAX, 2);

	//�i�ޗ\��̍��W��n���}��ϐ�
	int mediatorX = -1;
	int mediatorY = -1;

	for (int j = 0;j < DIRECTION_MAX;j++) {
		//�i�ޗ\��̍��W
		int x = characters[_characterNumber].position.x + directions[j][0];
		int y = characters[_characterNumber].position.y + directions[j][1];

		//�i�ޗ\��̍��W���ꎞ�I�ɕۑ�
		int tempX = x;
		int tempY = y;

		//������k�̃u���b�N���擾
		switch (j) {
		case DIRECTION_NORTH:y -= BLOCK_HEIGHT / 2; break;
		case DIRECTION_SOUTH:y += BLOCK_HEIGHT / 2; break;//-1��������
		case DIRECTION_WEST: x -= BLOCK_WIDTH / 2; break;
		case DIRECTION_EAST: x += BLOCK_WIDTH / 2; break;//-1��������
		}

		/*
			���W���u���b�N�̕�(����)�Ŋ��邱�ƂŃu���b�N�̍s��ԍ����擾����B
			���W�����ɂȂ�΍s��ԍ���-1��n���B
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
			���W�����H�O�ɏo�Ă��������]�肩��A�␳�ł���B�Ⴆ�΁A
			x = 21�Ȃ�21 % BLOCK_COLUMN_MAX = 0�A
			x = -1�Ȃ�20 % BLOCK_COLUMN_MAX = 20�A
		*/
		x = (x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
		y = (y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

		//_characterNumber�̑��ɋA��t���O�������Ă���Ώ������Ȃ�
		int character = getCharacter(x, y);
		if ((!characters[_characterNumber].goHome) && (character >= CHARACTER_PACMAN)) {
			//�p�b�N�}��������ΏՓˏ���
			if (character == CHARACTER_PACMAN) {
				crashBetweenCharacters(_characterNumber);
				return;
			}
			//�����X�^�[�̑��ɋA��t���O�������ĂȂ��Ȃ珜�O
			else {
				if (!characters[character].goHome) continue;
			}
		}

		//�ǂ����邩�ʉ߂����u���b�N�ł���΁A���O����
		else if ((blocks[y][x].type == BLOCK_TYPE_WALL) || (blocks[y][x].checked[_characterNumber])) continue;

		//�i�ޗ\��̍��W�ƖړI�n�̋������v�Z
		VEC2 distanceFromAToB = {
			_destination.x - x,
			_destination.y - y
		};
		double distance = pow(distanceFromAToB.x, 2) + pow(distanceFromAToB.y, 2);

		//4�u���b�N���r���ĒZ�����meditor�ɓn��
		if (distance < tempDistance) {
			tempDistance = distance;
			mediatorX = tempX;
			mediatorY = tempY;
		}
	}

	//�i�߂�u���b�N���Ȃ��Ȃ�A���[�g������������
	if ((mediatorX == -1) && (mediatorY == -1)) routeInit(_characterNumber);
	//���W���m�肵�āA�i�񂾃u���b�N�ɂ̓t���O�𗧂Ă�
	else {
		characters[_characterNumber].position.x = mediatorX;
		characters[_characterNumber].position.y = mediatorY;

		mediatorX = mediatorX / BLOCK_WIDTH;
		mediatorY = mediatorY / BLOCK_HEIGHT;
		blocks[mediatorY][mediatorX].checked[_characterNumber] = true;
	}
}

//�p�b�N�}���Ɍ��ނ��ꂽ�C�W�P�����X�^�[���A���̑O�ɓ��������ۂ̏���
void goHome(int _characterNumber) {
	//�ړI�n�ɒ��������̏���
	if ((characters[_characterNumber].position.x == inFrontOfNestPointX) &&
		(characters[_characterNumber].position.y == inFrontOfNestPointY))
	{
		//���ҋ@���ł̏�����Ԃɖ߂�
		characters[_characterNumber].position.x = 10 * BLOCK_WIDTH + BLOCK_WIDTH / 2;
		characters[_characterNumber].position.y = 12 * BLOCK_HEIGHT + BLOCK_HEIGHT / 2;
		characters[_characterNumber].monsterSpeed = MONSTER_SPEED;
		characters[_characterNumber].invincible = true;
		characters[_characterNumber].stayHome = true;
		characters[_characterNumber].goHome = false;

		//���ɑҋ@�ł���̂�3�C�ŁA4�C�ɂȂ�Ȃ�1�C�O�ɏo��
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

//�u���b�N��L�����N�^�[��`�悷��֐�
void Display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//���H�̕`��	
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

	//�L�����N�^�[�̕`��
	for (int j = CHARACTER_PACMAN;j < CHARACTER_MAX;j++) {

		int x = characters[j].position.x;
		int y = characters[j].position.y;

		//�p�b�N�}��
		if (j == CHARACTER_PACMAN) {
			glColor3ub(0xff, 0xff, 0x00);

			//�̂ƌ�
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
		//�����X�^�[
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

			//��
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
						glVertex2f((GLfloat)x + characterDiameter / 2 - characterDiameter * 2 / 4, (GLfloat)y + characterDiameter * 2 / 6);//���̒��S
						glVertex2f((GLfloat)x + characterDiameter / 2 - characterDiameter * 2 / 3, (GLfloat)y + characterDiameter / 2);
						glVertex2f((GLfloat)x + characterDiameter / 2 - characterDiameter * 3 / 4, (GLfloat)y + characterDiameter * 2 / 6);
						glVertex2f((GLfloat)x - characterDiameter / 2, (GLfloat)y + characterDiameter / 2);
						glVertex2f((GLfloat)x - characterDiameter / 2, (GLfloat)y);
					}
					glEnd();
				}
				glPopMatrix();
			}

			//��
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

//�L�����N�^�[�̓������������郁�C���̊֐�
void Idle(void) {
	if (timePasses(CLOCK_UPDATE)) {
		clockMode[CLOCK_UPDATE].lastClock = clock();

		//�Q�[���S�̂̏�����
		if (flag.init) Init();

		//���Ԑ���
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

		//�L�����N�^�[
		for (int i = CHARACTER_PACMAN;i < CHARACTER_MAX;i++) {
			//�p�b�N�}���̏���
			if (i == CHARACTER_PACMAN) {
				//�V�������W�ɐi�ނ��߂ɌÂ����W��ۑ�
				characters[i].lastPosition = characters[i].position;

				//�u���b�N�̒��S�ɂ���ΐi�H�ύX�ł���
				if (isOnThePoint(i)) {
					if (keysPressed['w']) characters[i].directionOfMovement = DIRECTION_NORTH;
					if (keysPressed['s']) characters[i].directionOfMovement = DIRECTION_SOUTH;
					if (keysPressed['a']) characters[i].directionOfMovement = DIRECTION_WEST;
					if (keysPressed['d']) characters[i].directionOfMovement = DIRECTION_EAST;

					int x = characters[i].position.x + directions[characters[i].directionOfMovement][0];
					int y = characters[i].position.y + directions[characters[i].directionOfMovement][1];

					int tempX = x;
					int tempY = y;

					//�w�肵�����p�̃u���b�N�Ɉړ�
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

					//�u���b�N���ǂ���Ȃ�
					if (blocks[y][x].type != BLOCK_TYPE_WALL) {
						//�u���b�N�Ƀ����X�^�[������ΏՓˏ���
						int character = getCharacter(x, y);
						if (character > CHARACTER_PACMAN)
							crashBetweenCharacters(character);

						/*
							�u���b�N�Ƀp���[�G�T������΃����X�^�[���C�W�P�āA
							�ǐՃ��[�h����꒣�胂�[�h�ɕς��
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
							�u���b�N�Ƀp���[�G�T���h�b�g������΃u���b�N����
							�����̕`��������āA�J�E���g����
						*/
						if (blocks[y][x].type != BLOCK_TYPE_NONE) {
							blocks[y][x].type = BLOCK_TYPE_NONE;
							characters[i].dotAndPowerCount--;
						}

						//���W�̊m��
						characters[i].position.x = tempX;
						characters[i].position.y = tempY;
					}
					//�ǂ�����Ό��݂̍��W�Ŏ~�܂�
					else characters[i].directionOfMovement = -1;

					//���߂������ɐi��ł���ꍇ
					if (characters[i].directionOfMovement != -1) {
						//�`��̍ہAtrue�Ȃ�����J����Afalse�Ȃ�������
						if (characters[i].moveAnimation) characters[i].moveAnimation = false;
						else characters[i].moveAnimation = true;
					}
				}

				//���H������p���[�G�T�ƃh�b�g���Ȃ��Ȃ�Ώ������t���O�𗧂Ă�
				if (characters[i].dotAndPowerCount <= 0) flag.init = true;
			}
			//�����X�^�[�̏���
			else {
				//���ɋA��t���O�������Ă���ꍇ
				if (characters[i].goHome) {
					//�V�������W�ɐi�ނ��߂ɌÂ����W��ۑ�
					characters[i].lastPosition = characters[i].position;

					//�ړI�n�ł��鑃�̑O�̍s��ԍ�
					VEC2 destination = {
						inFrontOfNestPointX / BLOCK_WIDTH,
						inFrontOfNestPointY / BLOCK_HEIGHT
					};

					//�u���b�N�����ɒ�������i�H�ύX
					if (isOnThePoint(i)) {
						goToDestination(i, destination);
						getDirectionOfMovement(i);
					}
				}
				else {
					//�꒣�胂�[�h
					if (flag.territoryMode) {
						//�V�������W�ɐi�ނ��߂ɌÂ����W��ۑ�
						characters[i].lastPosition = characters[i].position;

						//�u���b�N�����ɒ�������i�H�ύX
						if (isOnThePoint(i)) {
							goToDestination(i, territoryPosition[i]);
							getDirectionOfMovement(i);
						}
					}

					//�ǐՃ��[�h
					if (flag.chaseMode) {
						if (i == CHARACTER_RED) {
							//�ԐF�̖ړI�n�̓p�b�N�}���̃u���b�N

							//�V�������W�ɐi�ނ��߂ɌÂ����W��ۑ�
							characters[i].lastPosition = characters[i].position;

							//�p�b�N�}���̂���u���b�N
							VEC2 destination = {
								characters[CHARACTER_PACMAN].position.x / BLOCK_WIDTH,
								characters[CHARACTER_PACMAN].position.y / BLOCK_HEIGHT
							};

							//�u���b�N�����ɒ�������i�H�ύX
							if (isOnThePoint(i)) {
								goToDestination(i, destination);
								getDirectionOfMovement(i);
							}

						}

						if (i == CHARACTER_BLUE) {
							//���F�̖ړI�n�̓p�b�N�}���𒆐S�ɂ��ĐԐF�Ɠ_�Ώ̂ɂȂ�u���b�N

							//�V�������W�ɐi�ނ��߂ɌÂ����W��ۑ�
							characters[i].lastPosition = characters[i].position;

							//�u���b�N�����ɒ�������i�H�ύX
							if (isOnThePoint(i)) {
								//�p�b�N�}���̂���u���b�N
								int pacmanX = characters[CHARACTER_PACMAN].position.x / BLOCK_WIDTH;
								int pacmanY = characters[CHARACTER_PACMAN].position.y / BLOCK_HEIGHT;

								//�ԐF�̂���u���b�N
								int redX = characters[CHARACTER_RED].position.x / BLOCK_WIDTH;
								int redY = characters[CHARACTER_RED].position.y / BLOCK_HEIGHT;

								//�ԐF�ɑ΂���p�b�N�}���̑��΍��W
								VEC2 distanceFromAToB = {
									pacmanX - redX,
									pacmanY - redY,
								};

								//�Y���̓_�Ώ̂ɂȂ�ʒu
								int x = pacmanX + distanceFromAToB.x;
								int y = pacmanY + distanceFromAToB.y;

								//���H�O�ɏo��ΐ^���΂̈ʒu�ɖ߂�
								x = (x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
								y = (y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

								VEC2 destination = { x,y };

								goToDestination(i, destination);
								getDirectionOfMovement(i);
							}
						}

						if (i == CHARACTER_PINK) {
							//�s���N�F�̖ړI�n�̓p�b�N�}�����班���O�̃u���b�N

							//�V�������W�ɐi�ނ��߂ɌÂ����W��ۑ�
							characters[i].lastPosition = characters[i].position;

							//�u���b�N�����ɒ�������i�H�ύX
							if (isOnThePoint(i)) {
								VEC2 destination = { -1,-1 };

								//�p�b�N�}���̂���u���b�N
								int pacmanX = characters[CHARACTER_PACMAN].position.x / BLOCK_WIDTH;
								int pacmanY = characters[CHARACTER_PACMAN].position.y / BLOCK_HEIGHT;

								//�����O�̃u���b�N��3��
								const int frontOfPacmanPos = 3;//6

								//�p�b�N�}���̐i�s�����ɉ����ĖړI�n��I��
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
									//DIRECTION_NORTH�Ɠ���
									destination.y = pacmanY - frontOfPacmanPos;
									destination.x = pacmanX;
									break;
								}

								//���H�O�ɏo��ΐ^���΂̈ʒu�ɖ߂�
								destination.x = (destination.x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
								destination.y = (destination.y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

								goToDestination(i, destination);
								getDirectionOfMovement(i);
							}
						}

						if (i == CHARACTER_ORANGE) {
							/*
								�I�����W�F�̖ړI�n�́A�p�b�N�}���ƃI�����W�F�̋������r���āA
								������΃p�b�N�}���̃u���b�N�A�߂���΃p�b�N�}�����牓���̃u���b�N
							*/

							//�V�������W�ɐi�ނ��߂ɌÂ����W��ۑ�
							characters[i].lastPosition = characters[i].position;

							//�u���b�N�����ɒ�������i�H�ύX
							if (isOnThePoint(i)) {
								//�ǐՂ��邩�����邩�̊�ƂȂ鋗��
								const int chaseOrEscapeDistance = 3;

								//�p�b�N�}���̂���u���b�N
								int pacmanX = characters[CHARACTER_PACMAN].position.x / BLOCK_WIDTH;
								int pacmanY = characters[CHARACTER_PACMAN].position.y / BLOCK_HEIGHT;

								//�I�����W�F�̂���u���b�N
								int orangeX = characters[i].position.x / BLOCK_WIDTH;
								int orangeY = characters[i].position.y / BLOCK_HEIGHT;

								//�I�����W�F�ɑ΂���p�b�N�}���̑��΍��W
								double distanceFromAToB = pow(pacmanX - orangeX, 2) + pow(pacmanY - orangeY, 2);

								//�I�����W�F���p�b�N�}���Ƌ߂����
								if (distanceFromAToB < chaseOrEscapeDistance) {
									/*
										���݂̃I�����W�F�̃u���b�N����A������k�̃u���b�N�ŁA
										�ł��p�b�N�}�����牓���u���b�N��ړI�n�Ƃ���
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

									//���H�O�ɏo��ΐ^���΂̈ʒu�ɖ߂�
									destination.x = (destination.x + BLOCK_COLUMN_MAX) % BLOCK_COLUMN_MAX;
									destination.y = (destination.y + BLOCK_ROW_MAX) % BLOCK_ROW_MAX;

									goToDestination(i, destination);
								}
								//�I�����W�F���p�b�N�}���Ɖ������
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
			//���W��i�߂�
			if (characters[i].directionOfMovement != -1) {
				for (int count = 0;count < characters[i].monsterSpeed;count++) {
					//�V�������W�ɐi�ނ��߂ɌÂ����W��ۑ�
					characters[i].lastPosition = characters[i].position;

					//�t���O�������Ă�΁A���̑O�ɒ��������̃`�F�b�N
					if (characters[i].goHome) goHome(i);

					//�u���b�N�����ɒ�������i�H�ύX
					if (isOnThePoint(i)) break;
					else {
						//�L�����N�^�[���m�ŏՓ˂������Ȃ�i�H�ύX
						changeDirectionOfMovement(i);

						//���H�O�ɏo��ΐ^���΂̈ʒu�ɖ߂�
						overflowFromMap(i);

						//���W��i�߂�
						characters[i].position.x = characters[i].position.x + directions[characters[i].directionOfMovement][0];
						characters[i].position.y = characters[i].position.y + directions[characters[i].directionOfMovement][1];
					}
				}
			}
		}

		glutPostRedisplay();
	}
}

//�L�[�{�[�h��������Ă�Ȃ�true
void Keyboard(unsigned char _key, int, int) {
	keysPressed[_key] = true;
}

//�L�[�{�[�h��������ĂȂ��Ȃ�false
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