#include <Windows.h>
#include <ctime>
#include <iostream>
#include <GL\freeglut.h>
#include <soil.h>
#include <bass.h>
#include <fstream>

using namespace std;

struct piece
{
	int x1, y1, x2, y2, col, row, color, match, alpha; //свойства одной плитки
	piece() { match = 0; alpha = 255; } //устанавливаем в значения при создании
} grid[10][10]; //двухуровневый массив с плитками

struct record 
{
	char name[100]; //имя рекодтсмена
	int rec; //его колличество очков
} recx[11]; //колличество рекордой

POINT pos, //точка для запоминания где была нажата мыш в игре
	  coord{ 0,0 }, //точка мыши в меню
	  offset{ 270,70 }; //отступ поля с плитками от края экрана

bool isSwap = false, 
	 isMoving = false; //говорят за себя)

int t = 0, //просто нужна(обновление экрана в сек)
	x_1, y_1, x_2, y_2, //координаты 1 и 2 плитки на которые нажал пользователь для их обмена
	click = 0, //колличество нажатий мышки, состояние 1 и 2, 1 по первой плитке, 2 по второй
	score = 0, //счет в одной игре считается здесь
	width = 750, height = 600, //размеры окна
	s = 0, //состояние окна, 0- рисуеться меню, 1- игра, 2- таблица рекордой 
	count_texture = 9, //счетчик текстур
	soundrepeat = 3, //просто нужно чтоб не повторялась музяка для склабывания нескольких плиток
	ts = 45,//размер плиток в пикселях
	colrow = 8; //колличество столбцов и колонок

time_t a, b; //засечение времени чтоб пользователь играл только минуту и не больше
char nameplayer[100]; //имя играющего пользователя

LPCSTR text[9] = { "RS_bg.png", "menu_1.png", "menu_2.png", "menu_3.png" , "Color-1.png", "Color-2.png", "Color-3.png", "Color-4.png", "Color-5.png" };//картинки текстур
GLuint etd[9];//хранение индексов уже после загрузки текстур

void playbg(int asd); //проигрывание музыки выбор по номеру
void sortrecx(); //сортировка массива с записями об таблице рекордов
void loadatfile(); //считывание в массив с файла рекордов
void saveinfile(); //запись в файл рекордов
void swap(piece p1, piece p2); //обмен 2-ух обектов в массиве с плитками местами
void DrawQuads(double x1, double x2, double y1, double y2, GLuint texter, int alpha); //рисование квадрата по координатам, наложение не этот квадрат текстуры и передача  прозрачности фигуры
GLuint LoadGLTextures(LPCSTR nametext); //загрузка текстур
void DrawLines(double x1, double x2, double y1, double y2); //рисование линии по 2-ум координатам
void mouse(int button, int state, int x, int y); //функция обработки нажатия мышы(стандарт openGL)
int menu(int s); // обработка нажатий в s=0 меню
void display(); // отображение и перерисовка окна по 3-ем состояниям

void swap(piece p1, piece p2)
{
	std::swap(p1.col, p2.col);
	std::swap(p1.row, p2.row);

	grid[p1.row][p1.col] = p1;
	grid[p2.row][p2.col] = p2;
}

GLuint LoadGLTextures(LPCSTR nametext)
{
	GLuint texture = SOIL_load_OGL_texture(nametext, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y); //загрузка из файла в память текстур
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	return texture;
}

void DrawQuads(double x1, double x2, double y1, double y2, GLuint texter, int alpha)
{
	if (texter != NULL) {
		glEnable(GL_TEXTURE_2D); //подключение текстур
		glBindTexture(GL_TEXTURE_2D, texter); //пременение той текстуры которой индекс передали
		glEnable(GL_BLEND); //подключение прозрачности
		glColor4f(0, 0, 0, alpha / 255.0); //задаем прозрачность
 		glBegin(GL_QUADS); //рисуем квадрат
		glTexCoord2d(0, 1);	glVertex2d(x1, y1);
		glTexCoord2d(1, 1);	glVertex2d(x2, y1);
		glTexCoord2d(1, 0);	glVertex2d(x2, y2);
		glTexCoord2d(0, 0);	glVertex2d(x1, y2);
		glEnd(); //окончание рисования
		glDisable(GL_TEXTURE_2D);//выключение текстур
		glDisable(GL_BLEND); //выключение прозрачности
	}
	else //рисуем просто квадрат если нет текстур 
	{
		glBegin(GL_QUADS); 
		glVertex2d(x1, y1);
		glVertex2d(x2, y1);
		glVertex2d(x2, y2);
		glVertex2d(x1, y2);
		glEnd();
	}
	return;
}

void DrawLines(double x1, double x2, double y1, double y2) 
{
	//тут и так все ясно
	glBegin(GL_LINES);
	glVertex2d(x1, y1);
	glVertex2d(x2, y2);
	glEnd();
}

void mouse(int button, int state, int x, int y)
{
	if (s == 0) //нажатие мыши в меню
	{
		if ((button == GLUT_LEFT_BUTTON)&(state == GLUT_DOWN))
		{
			coord.x = x;
			coord.y = y;
			t = clock();
		}
	}
	if (s == 1) //нажатие мыши в самой игре
	{
		if ((button == GLUT_LEFT_BUTTON)&(state == GLUT_DOWN))
		{
			if (x >= 315 && x <= 675 && y >= 70 && y <= 430) //фильтруем от мискликов
			{
				if (!isSwap && !isMoving)
					click++;
				pos.x = x - offset.x;
				pos.y = y - offset.y;
				t = clock();
			}
		}
	}
	glutPostRedisplay();
}

int menu(int s)
{
	if (!s)
	{
		DrawQuads(335, 415, 210, 240, etd[1], 255);//рисуем 3 кнопки в меню и налаживаем текстуры на них
		DrawQuads(335, 415, 250, 280, etd[2], 255);
		DrawQuads(335, 415, 290, 320, etd[3], 255);
	}

	if (coord.x > 335 && coord.x < 415)
	{
		if (coord.y > 210 && coord.y < 240) //нажатие 1-ого пункта меню
		{
			cout << "Input name: ";
			cin >> nameplayer; //просит ввести имя игрока чтоб потом добавить в таблицу рекордов
			score = 0;//обнуление переменной с рекордом
			s = 1; //состояние игры == 1, значит начинаем играть и рисуеться поле с игрой
			coord.x = 0;
			coord.y = 0;
			cout << "You have 5 second for change window and you game has START!" << endl;
			Sleep(1000);
			cout << "-- 4 --" << endl;
			Sleep(1000);
			cout << "-- 3 --" << endl;
			Sleep(1000);
			cout << "-- 2 --" << endl;
			Sleep(1000);
			cout << "-- 1 --" << endl;
			system("cls");
			time(&a);
		}

		if (coord.y > 250 && coord.y < 280) //нажатие 2-ого пункта меню
		{
			s = 2;// 2 состояние приложение открытие таблицы рекорда
			coord.x = 0;
			coord.y = 0;
		}

		if (coord.y > 290 && coord.y < 320)// нажатие 3-его пункта меню -> выход из приложения 
		{
			exit(EXIT_SUCCESS);
		}
	}

	return s;
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);
	DrawQuads(0, width, 0, height, etd[0], 255); //рисование фона в любом состояние окна 

	if (!s)
	{
		s = menu(s); //отрисовка меню и обработка нажатий
	}

	if (s == 1) //открисовка игры, состояние игра
	{
		time(&b);
		if (difftime(b, a) > 60) //засечение времени на 60 сек, после чего добовляем рекорд и имя в массив, который потом сортируеться и только 10 записей поподают в файл и таблицу
		{
			strcpy_s(recx[10].name,nameplayer);
			recx[10].rec = score;
			sortrecx();
			saveinfile();
			s = 0;//состояние 0-меню приложнния, после чего открываеться меню
		}


		char str[100] = "Score: ", str1[100];
		_itoa_s(score, str1, 10);
		strcat_s(str, 100, str1);
		glRasterPos2f(90, 90);
		for (int i = 0; str[i] != '\0'; i++)
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str[i]); //вывод счета


		char str2[100] = "Timeleft: ", str3[100];
		_itoa_s(60 - difftime(b, a), str3, 10);
		strcat_s(str2, 100, str3);
		glRasterPos2f(90, 120);
		for (int i = 0; str2[i] != '\0'; i++)
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str2[i]); //вывод оставшегося времени


		if (click == 1)//запись координат 1-ого нажатия мыши и выяснение положения плитки на которую нажали
		{
			x_1 = pos.x / ts;
			y_1 = pos.y / ts + 1;
		}
		if (click == 2)//запись координат 2-ого нажатия мыши и выяснение положения плитки на которую нажали во 2-ую очередь
		{
			x_2 = pos.x / ts;
			y_2 = pos.y / ts + 1;
			if (abs(x_1 - x_2) + abs(y_1 - y_2) == 1)//если между плитками расстояние в 1 единицу то обмен местами возможен
			{
				swap(grid[y_2][x_2], grid[y_1][x_1]);
				playbg(3);//проигрывание музыки свапа
				isSwap = 1;
				click = 0;
			}
			else
				click = 1;
		}

		//поиск 3+ в ряд или столбец
		for (int i = 1; i <= 8; i++)
			for (int j = 1; j <= 8; j++)
			{
				if (grid[i][j].color == grid[i + 1][j].color)
					if (grid[i][j].color == grid[i - 1][j].color)
						for (int n = -1; n <= 1; n++) grid[i + n][j].match++;

				if (grid[i][j].color == grid[i][j + 1].color)
					if (grid[i][j].color == grid[i][j - 1].color)
						for (int n = -1; n <= 1; n++) grid[i][j + n].match++;
			}

		//анимация передвижения плиток
		isMoving = false;
		for (int i = 1; i <= 8; i++)
			for (int j = 1; j <= 8; j++)
			{
				piece &p = grid[i][j];
				int dx, dy;
				for (int n = 0; n < 10; n++)   // 10 - скрость анимации
				{
					dx = p.x1 - p.col*ts - offset.x;
					dy = p.y1 - p.row*ts - offset.y;
					if (dx)
					{
						p.x1 -= dx / abs(dx);
						p.x2 -= dx / abs(dx);
					}
					if (dy)
					{
						p.y1 -= dy / abs(dy);
						p.y2 -= dy / abs(dy);
					}
				}
				if (dx || dy) isMoving = 1;
			}

		//анимация удаления плиток
		if (!isMoving) {
			for (int i = 1; i <= 8; i++)
				for (int j = 1; j <= 8; j++)
					if (grid[i][j].match) {
						if (grid[i][j].alpha > 10)
						{
							grid[i][j].alpha -= 80;
							isMoving = true;
						}
						if (grid[i][j].alpha<10) {
							playbg(2);
						}
					}
		}

		//получение счета для проверки 
		int addscore = 0; //переменна для проверки
		for (int i = 1; i <= 8; i++)
			for (int j = 1; j <= 8; j++)
			{
				addscore += grid[i][j].match;
			}

		//второй обмен местами плиток если они не могут в том месте исчезнуть
		if (isSwap && !isMoving)
		{
			if (!addscore) //использование перемнно для проверки
			{
				swap(grid[y_2][x_2], grid[y_1][x_1]);
				playbg(3);
			}
			isSwap = 0;
		}

		//обновление таблицы с плитками
		if (!isMoving)
		{
			for (int i = 8; i > 0; i--)
				for (int j = 1; j <= 8; j++)
					if (grid[i][j].match)
						for (int n = i; n > 0; n--)
							if (!grid[n][j].match)
							{
								score += grid[i][j].match;//прибавление к основному счету
								swap(grid[n][j], grid[i][j]);
								break;
							};

			for (int j = 1; j <= 8; j++)//создание и добавление на поле новых плиток
				for (int i = 8, n = 0; i > 0; i--)
					if (grid[i][j].match)
					{
						grid[i][j].color = rand() % 5;
						grid[i][j].y1 = -ts * n++;
						grid[i][j].y2 = -ts * n++;
						grid[i][j].match = 0;
						grid[i][j].alpha = 255;
					}
		}

		for (int i = 1; i <= 8; i++)//рисование плиток
			for (int j = 1; j <= 8; j++)
			{
				piece p = grid[i][j];
				DrawQuads(p.x1, p.x2, p.y2, p.y1, etd[p.color + 4], p.alpha);
			}
	}

	if (s == 2) //отрисовка таблицы рекордова, состояние таблица рекордов
	{
		glLineWidth(3);
		glColor3d(0, 0, 1);
		for(int q=0;q<9;q++)
			DrawLines(350, 650, 150+q*50, 150+q*50);
		DrawLines(400, 400, 100, 600);
		DrawLines(500, 500, 100, 600);
		//до этого момента рисуется сама табличка

		char str1[100];
		for (int qw = 0; qw < 10; qw++) //вывод рекордов на экран
		{	
			_itoa_s(qw+1, str1, 10);
			glRasterPos2f(370, 130+qw*50);
			for (int i = 0; str1[i] != '\0'; i++)
				glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str1[i]);//вывод нумерации

			glRasterPos2f(410, 130 + qw * 50);
			for (int i = 0; recx[qw].name[i] != '\0'; i++)
				glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, recx[qw].name[i]);//вывод имен

			_itoa_s(recx[qw].rec, str1, 10);
			glRasterPos2f(520, 130 + qw * 50);
			for (int i = 0; str1[i] != '\0'; i++)
				glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str1[i]); //вывод счета каждого игрока
		}
	}

	if (GetAsyncKeyState(VK_RSHIFT))//если нажать на shift  в любом состояние игры то откроеться меню, такой себе выход из игры или из таблицы рекордов
		s = 0;

	glFlush();
}

void timer(int = 0)
{ //стандартна opengl функция, обновление экрана в секунду
	display();
	
	glutTimerFunc(50, timer, 0);
}

void playbg(int asd)
{
	if (asd == 1)
	{
		BASS_Init(-1, 44100, BASS_DEVICE_3D, 0, NULL); //Инициализация звукового потока
		HSTREAM str = BASS_StreamCreateFile(FALSE, "main.wav", 0, 0, 0); //Создаем поток str. Расположение нашего файла MP3
		BASS_ChannelPlay(str, FALSE);
	}
	if (asd == 2) {
		if (soundrepeat == 0) 
		{
			BASS_Init(-1, 44100, BASS_DEVICE_3D, 0, NULL); 
			HSTREAM str = BASS_StreamCreateFile(FALSE, "match.wav", 0, 0, 0); 
			BASS_ChannelPlay(str, FALSE);
			soundrepeat = 3;
		}
		soundrepeat--;
	}
	if (asd == 3) {
		BASS_Init(-1, 44100, BASS_DEVICE_3D, 0, NULL); 
		HSTREAM str = BASS_StreamCreateFile(FALSE, "swipe.wav", 0, 0, 0);
		BASS_ChannelPlay(str, FALSE);
	}
}

void saveinfile() 
{	
	fstream fin("textrecord.txt", ios_base::out| ios_base::trunc);
	char buff[50];
	for (int qw = 0; qw < 10;qw++)
	{
		fin << recx[qw].name << " " << recx[qw].rec << endl;
	}
	fin.close();
	return;
}

void loadatfile() 
{
	fstream fout("textrecord.txt", ios_base::in);
	char buff[50];
	for (int qw = 0; qw < 10; qw++)
	{
		fout >> buff;
		strcpy_s(recx[qw].name, buff);
		fout >> buff;
		recx[qw].rec = atoi(buff);
	}
	fout.close();
	return;
}

void sortrecx() 
{
	record temp;
	for (int i = 0; i < 11 - 1; i++) {
		for (int j = 0; j < 11 - i - 1; j++) {
			if (recx[j].rec < recx[j + 1].rec) {
				temp = recx[j];
				recx[j] = recx[j + 1];
				recx[j + 1] = temp;
			}
		}
	}
}

int main(int argc, char** argv)
{
	loadatfile();
	sortrecx();//загрузка рекордов из файла и их сортировка

	srand(static_cast<unsigned int>(time(NULL)));//запуска псевдорандома
	playbg(1); //проигрывание основной мелодии
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(200, 200);
	glutCreateWindow("Bejeweled"); //до сюда создание самого окна
	glClearColor(0, 0, 0, 1.0);

	gluOrtho2D(0, width, height, 0);//задаем координатные прямые чтоб потом рисовать правильно

	for (int a = 0; a < count_texture; a++) 
	{
		etd[a] = LoadGLTextures(text[a]); //загрузка текстур
	}

	for (int i = 1; i <= 8; i++) //создание таблицы плиток, заполнение структуры каждой плитки, ее свойста
		for (int j = 1; j <= 8; j++)
		{
			grid[i][j].color = rand() % 5;
			grid[i][j].x1 = ts * j + offset.x;
			grid[i][j].y1 = ts * i + offset.y;
			grid[i][j].x2 = grid[i][j].x1 + ts;
			grid[i][j].y2 = grid[i][j].y1 - ts;
			grid[i][j].col = j;
			grid[i][j].row = i;
		}

	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutTimerFunc(50, timer, 0);
	glutMainLoop();

	return 0;
}