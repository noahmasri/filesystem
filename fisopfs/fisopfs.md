# fisop-fs

Lugar para respuestas en prosa y documentación del TP.

# decisiones de diseno
Para el presente trabajo se eligio llevar a cabo una estructura basada en vectores estaticos. Se tienen dos vectores principales, uno de archivos y uno de directorios. A su vez, se cuenta con dos vectores mas que contiene los indices de las posiciones de los vectores previamente mencionados que no estan ocupadas.

## archivos
Para modelar los archivos decidimos tener los siguientes campos:
* nombre: un string estatico en donde guardamos el nombre del archivo. la capacidad maxima se definio en base al tamano de path maximo soportado por los sistemas unix-like, definido en limits.h. se guarda igualmente su path relativo.
* contenido: los datos que el usuario quiere almacenar en el archivo. la capacidad maxima se definio en base al tamano usual de los bloques. al querer tener una solucion simple, y siendo un sistema de archivos pequeno, decidimos que eso seria mas que suficiente.
* stats: la metadata del archivo, que sera explicada luego.
* parent_dir: el indice de donde se encuentra el directorio padre en el arreglo de directorios. si es cero entonces se trata del directorio raiz. se decidio guardar este campo con el fin de facilitar la eliminacion de un archivo, pues el directorio padre almacena una referencia a todos los archivos que le corresponden.

Dado que los stats tienen muchos campos como la cantidad de bloques y etc que no conocemos, decidimos guardar solo la informacion que nos resulta a nosotros relevante y es nuestro deber llevar la cuenta. Los campos que tiene este son el tiempo de modificacion y de acceso, que se modifican en write y en read y write respectivamente, el tamano del archivo, que seria el largo del vector de contenido, el uid y el gid, los cuales no son tan relevantes de guardar pues siempre son los mismos, pero por las dudas guardamos los responsables de la creacion, y por ultimo el modo, que contiene el tipo de archivo y los permisos, por mas que no estemos chequeandolos.

## directorios
Para modelar los directorios, decidimos tener los siguientes campos:
* nombre: del mismo tamano y con la misma justificacion que los archivos.
* stats: la metadata del directorio previamente explicada. es la misma que la usada para archivos.
* un vector de posiciones de archivos: se almacenan en este los indices de todos los archivos que pertenecen a este directorio, para facilitar su recorrido y la busqueda de un archivo que contiene el nombre de este en su path.
* un contador de archivos: se lleva la cuenta de la cantidad de archivos que estan dentro de este directorio, tanto para el recorrido del vector anterior, para no tener que llegar hasta el final, como para la eliminacion. al eliminar, se sustituye al elemento eliminado del vector con el ultimo elemento ocupado, y se resta el contador.
* ocupado: al no tener referencia de los directorios y manejarnos por una lista de posiciones de directorios libres, optamos por en vez de guardar una lista de directorios ocupados, senalarlos con un bool a los que contienen informacion valida. 

detalle de implementacion: al tener almacenada tambien la cantidad de directorios ocupados, para intentar reducir un poco la complejidad de la busqueda de un directorio, se suele hacer uso de un contador de directorios validos recorridos, y una vez que ese contador llega a la totalidad de los directorios deja de recorrer. en la inicializacion, los indices de los directorios se insertan en la lista de directorios libres de mayor a menor, con el fin de ir agarrando siempre los de menores indices. una vez eliminado un directorio, su indice se inserta al final del vector de posiciones libres nuevamente, por lo que la solucion, por mas rustica, resulta bastante eficiente. 

para la eliminacion de archivos se hace una operacion analoga, solo que en estos no se lleva cuenta de los recorridos ya que se los recorre mediante su padre. en estos se debe tener el cuidado de eliminar tambien la referencia en su directorio padre como ya fue antes mencionado.

## directorio raiz
nuestra solucion en un principio contaba con un directorio raiz modelado por separado, en el que se guardaban entries que podian ser tanto archivos como directorios. terminamos descartando esta solucion ya que no solo agregaba complejidad a la eliminacion sino que al estar todos los directorios en el directorio raiz esto no era necesario. es por esto que decidimos modelar al directorio raiz como un directorio exactamente igual que los otros, con el cuidado de recorrer, ademas de los archivos a los que guarda referencia, todos los directorios que tiene dentro al llegar un pedido de lectura del directorio, o al tener que buscar en este. este directorio se guarda en la posicion 0 y se inicializan sus stats en la funcion init. 

## busqueda de archivos
Para encontrar un archivo específico dado un path:
* usamos la funcion auxiliar split_line para descomponer el path. Como solo permitimos un nivel de profundidad de 
directorios, separamos el path en cada "/", dos veces. Si no se encuentra separadador el string resultante es NULL.
* llamamos a la funcion find_file_index con los dos strings obtenidos y separamos en dos caso: 
    1- Esta en el root.
    2- Esta dentro de otro directorio.
* Si estamos en el caso 1 ya podemos acceder al directorio ya que sabemos que su indice en el vector de directorios es 0, por la convencion definida por nosotros. 
* Si estamos en el caso 2, llamamos a search_for_directory con el primer string, la cual nos devuelve el indice  en el vector de directorios.
* llamamos a la funcion search_in_dir con el indice del directorio y con el string que contiene el nombre del archivo (primero en caso 1, y segundo en caso 2) y nos devuelve el indice del archivo dentro del vector de archivos del directorio. Con esto podemos acceder a el campo file_pos del vector de archivos del directorio y obtenemos finalmente el indice  en el vector de archivos.

Ejemplo: busco "cami.txt"
* primer_string = cami.txt,segundo_string = NULL.
* Entro a caso 1, recorro el vector de archivos del directorio[0] en search_in_dir y chequeo si coincide el nombre con "cami.txt". Si coincide me guardo de esa entrada su posicion en el vector global en la variable index.
* El archivo, si existe esta en files[index]. Si no se encuentra index = -1;

Ejemplo: busco "manu/cami.txt"
* primer_string = manu,segundo_string = cami.txt
* Entro a caso 2, recorro el vector de directorios en search_directory y chequeo si esta ocupado y si su nombre coincide con "manu". Una vez que recorri la misma cantidad de ocupados que los definidos en directory_count dejo de buscar. Obtentengo dir_index y dir_index = -1 si no se lo encontro.
*  recorro el vector de archivos del directorio[dir_index] en search_in_dir y chequeo si coincide el nombre con "cami.txt". Si coincide me guardo de esa entrada su posicion en el vector global en la variable index.
* El archivo, si existe esta en files[index]. Si no se encuentra index = -1;

## formato de persistencia
Para guardar los archivos generados al momento de desmontar el filesystem o bien cuando ocurre una llamada a fflush, decidimos guardar las stats en un formato pseudo csv especificando en una misma linea la metadata del archivo o directorio a ser guardado, empezando por su nombre y siguiendo con sus stats (mode, ids, size y tiempos de acceso/modificacion). Luego, una linea debajo, se registra el contenido del archivo acompañado por un string __ENDCONTENT__ para indicar la finalizacion del mismo. Ademas, por cada archivo o directorio a ser guarda se pone en la misma linea de su metadata el tipo al que pertenece (FILE, DIR). Respecto al orden de registro, primero se registran los archivos guardados en el directorio root, para luego registrar cada directorio (todos se encuentran en el root), escribiendo primero la metadata del directorio y su contenido en el formato aclarado anteriormente, siguiendo con los archivos que este contiene, y una vez finalizado todos los archivos del directorio actual se continua con el resto.

Se encuenta en el archivo persistencia.txt un ejemplo del mismo, en donde siguiendo con lo explicado anteriormente, tendremos a los archivos shakira.txt y pique.txt en el root, y luego tendremos en el directorio hamilton el archivo saminamina.txt y claramente.txt, y en el directorio el_biza el archivo wakawaka.txt.

## limites de cantidad de archivos/directorios
En nuestra implementacion, al soportar un unico nivel de recursion en los directorios, el limite de cantidad de directorios que tenemos es el mismo que figura en el arreglo de directorios totales que es 500. Para los archivos es una caso diferente, ya que los archivos pueden encontrarse tanto dentro del directorio root como dentro de otro directorio en un nivel mayor de profundidad, el limite de que figura en el arreglo de archivos (1000) se refiere a la cantidad total de archivos que soporta el filesystem, incluyendo tanto los que se encuentra en el directorio root como los que se encuentran en otro. No importa si estan distribuidos a lo largo de todo el sistema o estan concentrados en un mismo directorio, mientras que no se pase el limite de archivos totales pueden guardarse cuantos se requieran en cualquiera de los directorios. Consideramos que tener un promedio de dos archivos por directorio suena razonable. Dejamos en manos del usuario el alcanzar o no este limite, ya que consideramos que en un unico nivel 500 directorios suena mas que suficiente.

## problemas con ls
Estuvimos intentando solucionar un error en la ejecucion de ls, sin mucho exito. Al ejecutar ls estando parados sobre el directorio raiz, notamos que previo a imprimir los directorios y archivos que este contiene nos aparece un warning que dice "ls: leyendo el directorio '.': Error de entrada/salida". Si se ejecuta ls -al vemos que si bien este warning aparece, la metadata del directorio "." se muestra apropiadamente, al igual que todos el resto de los archivos y directorios que este contiene, pero no logramos deshacernos de dicha advertencia.

## pruebas
A la hora de correr las pruebas, asegurarse de que el archivo en el que se persiste este vacio ya que sino estas fallaran.