# Mapa do código e das funções

As linhas referem-se aos arquivos deste repositório (versão comentada). Clique na função para abrir o código no GitHub. O JavaScript foi extraído de pagina.h apenas para estudo; o firmware continua usando pagina.h.

## esp32_principal/esp32_principal.ino

| Função | Linha | O que faz |
|---|---|---|
| [tarefaLCD](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L55) | 55 | Recebe mensagens da fila, verifica o barramento, inicializa/reinicializa o LCD e escreve linhas alteradas; roda na tarefa dedicada. |
| [iniciarLCD](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L72) | 72 | Cria a fila de uma mensagem e a tarefa; trata a falha de criação sem colocar a escrita do LCD no ciclo principal. |
| [publicarLCD](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L78) | 78 | Produz uma cópia de duas linhas de 16 caracteres e troca a mensagem pendente apenas quando o conteúdo muda. |
| [atender](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L98) | 98 | Entrega uma conexão com dados à biblioteca WebServer e aplica TCP_NODELAY no cliente aceito. |
| [handleClient](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L106) | 106 | Prioriza clientes com dados, mantém uma fila limitada de conexões ociosas e delega o parser à biblioteca. |
| [ehCamera](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L162) | 162 | Consulta /status de um IP e verifica se a resposta contém framesize; é identificação simples por formato. |
| [procurarCamera](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L175) | 175 | Tenta o IP padrão e endereços do mesmo prefixo local até encontrar uma resposta de câmera. |
| [dataAgora](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L195) | 195 | Formata a data/hora local se o relógio for válido; caso contrário informa ausência de sincronização. |
| [registrar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L203) | 203 | Preenche a próxima posição do histórico circular, atribui identidade e marca o evento para persistência. |
| [fecharOcorrencia](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L216) | 216 | Fecha o agrupamento atual e atualiza seus dados, sem confundi-lo com os outros eventos do histórico. |
| [atualizarOcorrencia](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L227) | 227 | Traduz as mudanças do agrupador em criação, atualização ou encerramento de um Evento. |
| [responder](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L244) | 244 | Envia JSON com o status HTTP indicado e orientação para não reutilizar uma resposta em cache. |
| [status](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L251) | 251 | Constrói o estado atual em JSON, incluindo modos, contadores, saúde, câmera e programação. |
| [historico](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L277) | 277 | Serializa os eventos retidos, na ordem do histórico circular, para o navegador. |
| [guardarOperacao](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L287) | 287 | Codifica modo/pausa e confirma a gravação antes de aceitar a alteração. |
| [carregarOperacao](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L293) | 293 | Recupera e valida modo/pausa; usa o padrão quando não consegue confirmar a configuração. |
| [silenciar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L303) | 303 | Para o controle do alarme, baixa o pino e registra o desligamento se havia som ativo. |
| [relogioNoturno](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L310) | 310 | Converte a hora Unix válida em minutos locais de Brasília; rejeita relógio não sincronizado. |
| [horarioNoturno](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L311) | 311 | Converte minutos do dia para o texto HH:MM. |
| [guardarNoturno](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L313) | 313 | Valida e grava o bloco de configuração noturna; só troca o estado em RAM após sucesso. |
| [carregarNoturno](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L319) | 319 | Lê o bloco de 20 bytes, confere formato/CRC e recupera os horários e a suspensão manual. |
| [suspenderNoturno](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L326) | 326 | Salva a faixa em que uma escolha manual deve prevalecer, inclusive se ainda falta sincronizar o relógio. |
| [aplicarNoturno](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L331) | 331 | Avalia horário e suspensão, preserva a pausa e, quando necessário, salva e aplica a troca de modo. |
| [statusNoturno](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L343) | 343 | Descreve configuração, relógio, faixa, ajuste manual e resultado de salvamento/aplicação em JSON. |
| [comando](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L355) | 355 | Valida ações HTTP para programação, modo, pausa e sirene; trata falhas de salvamento e entradas inválidas. |
| [setup](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L389) | 389 | Inicialização. Na principal prepara hardware, dados, Wi-Fi e rotas; na câmera prepara o sensor e os servidores. |
| [loop](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/esp32_principal.ino#L418) | 418 | Execução repetida. Na principal coordena as tarefas; na câmera apenas informa o IP enquanto os servidores trabalham em suas tarefas. |

## esp32_principal/ocorrencia.h

| Função | Linha | O que faz |
|---|---|---|
| [encerrar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/ocorrencia.h#L42) | 42 | Encerra explicitamente o estado do agrupador. |

## esp32_principal/modos.h

| Função | Linha | O que faz |
|---|---|---|
| [idModo](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L8) | 8 | Devolve o identificador técnico estável do modo, usado em JSON e comandos. |
| [nomeModo](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L9) | 9 | Devolve o nome legível do modo. |
| [lerModo](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L10) | 10 | Aceita apenas identificadores de modo reconhecidos. |
| [tempoSom](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L12) | 12 | Mapeia modo para 0, 8000 ou 250 milissegundos. |
| [codificar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L17) | 17 | Converte um estado para a representação persistente do módulo: byte de operação, registro de evento ou bloco noturno. |
| [carregar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L18) | 18 | Recupera e valida a representação persistida antes de aceitar os dados. |
| [parar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L24) | 24 | Marca o controle da sirene como inativo. |
| [manual](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L25) | 25 | Na sirene, inicia um teste quando o modo permite som; no noturno, verifica a suspensão por escolha manual. |
| [movimento](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L26) | 26 | Autoriza disparo de som somente para ocorrência nova, registro ligado e modo com som. |
| [atualizar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/modos.h#L27) | 27 | No agrupador, processa o sinal e devolve Inicio/Atualizacao/Fim/Nenhuma; na sirene, confere se o tempo de som terminou. |

## esp32_principal/historico.h

| Função | Linha | O que faz |
|---|---|---|
| [tipo](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/historico.h#L29) | 29 | Converte código numérico persistido para um tipo de evento reconhecido. |
| [codigo](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/historico.h#L33) | 33 | Converte o nome do tipo de evento para o número persistido. |
| [u32](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/historico.h#L34) | 34 | Grava ou recupera um inteiro de 32 bits com organização explícita de bytes. |
| [u32](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/historico.h#L35) | 35 | Grava ou recupera um inteiro de 32 bits com organização explícita de bytes. |
| [crc](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/historico.h#L37) | 37 | Calcula CRC32 para verificar integridade; não criptografa os dados. |
| [maisNovo](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/historico.h#L39) | 39 | Compara gerações de escrita considerando a representação de 32 bits. |
| [codificar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/historico.h#L41) | 41 | Converte um estado para a representação persistente do módulo: byte de operação, registro de evento ou bloco noturno. |
| [decodificar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/historico.h#L50) | 50 | Valida tamanho, assinatura, versão, flags e CRC e reconstrói o evento sem usar ponteiros persistidos. |

## esp32_principal/persistencia.h

| Função | Linha | O que faz |
|---|---|---|
| [chaveHistorico](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/persistencia.h#L14) | 14 | Cria a chave NVS correspondente a uma posição do histórico, como e00. |
| [marcarHistorico](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/persistencia.h#L15) | 15 | Assinala qual evento precisa ser salvo e se a alteração é urgente. |
| [historicoPendente](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/persistencia.h#L16) | 16 | Verifica se há alguma posição alterada aguardando gravação. |
| [formatarEventoSalvo](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/persistencia.h#L18) | 18 | Transforma o horário Unix em data textual, ou indica horário desconhecido. |
| [carregarHistorico](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/persistencia.h#L27) | 27 | Lê registros válidos, recupera contadores, ordena eventos e trata ocorrências interrompidas por reinício. |
| [sincronizarDatasHistorico](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/persistencia.h#L68) | 68 | Reconstrói horários de eventos da sessão corrente depois que a hora real fica disponível. |
| [salvarHistorico](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/persistencia.h#L79) | 79 | Agrupa gravações pendentes, aplica as janelas temporais e verifica sucesso de cada registro. |

## esp32_principal/migracao_memoria.h

| Função | Linha | O que faz |
|---|---|---|
| [abrirHistorico](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/migracao_memoria.h#L7) | 7 | Abre a área dedicada e realiza migração do namespace anterior sem apagar a origem. |

## esp32_principal/noturno.h

| Função | Linha | O que faz |
|---|---|---|
| [valida](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L16) | 16 | Confere intervalos, horários diferentes e o modo diurno permitido. |
| [noite](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L18) | 18 | Testa se o minuto local está no intervalo, usando E ou OU conforme ele atravessa a meia-noite. |
| [periodo](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L20) | 20 | Identifica a última transição em minutos absolutos para manter o ajuste manual através da meia-noite. |
| [manual](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L27) | 27 | Na sirene, inicia um teste quando o modo permite som; no noturno, verifica a suspensão por escolha manual. |
| [crc](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L29) | 29 | Calcula CRC32 para verificar integridade; não criptografa os dados. |
| [u32](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L30) | 30 | Grava ou recupera um inteiro de 32 bits com organização explícita de bytes. |
| [ler32](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L31) | 31 | Lê um inteiro de quatro bytes na codificação do noturno. |
| [codificar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L33) | 33 | Converte um estado para a representação persistente do módulo: byte de operação, registro de evento ou bloco noturno. |
| [carregar](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L37) | 37 | Recupera e valida a representação persistida antes de aceitar os dados. |
| [lerHorarioNoturno](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/noturno.h#L44) | 44 | Aceita somente HH:MM válido e o converte em minutos. |

## esp32_principal/pagina_http.h

| Função | Linha | O que faz |
|---|---|---|
| [enviarPagina](https://github.com/lyralucas21/rastro/blob/main/esp32_principal/pagina_http.h#L9) | 9 | Calcula Content-Length e envia a página da flash em partes, substituindo __CAM__ sem uma String maior que o limite. |

## esp32_principal/config.h

Arquivo de configuração, mapeamento de pinos ou recurso auxiliar; a explicação está no guia e no próprio código.

## esp32_principal/partitions.csv

Arquivo de configuração, mapeamento de pinos ou recurso auxiliar; a explicação está no guia e no próprio código.

## esp32_principal/historico_inicial.h

Arquivo de configuração, mapeamento de pinos ou recurso auxiliar; a explicação está no guia e no próprio código.

## CameraWebServer/CameraWebServer.ino

| Função | Linha | O que faz |
|---|---|---|
| [setup](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/CameraWebServer.ino#L36) | 36 | Inicialização. Na principal prepara hardware, dados, Wi-Fi e rotas; na câmera prepara o sensor e os servidores. |
| [loop](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/CameraWebServer.ino#L188) | 188 | Execução repetida. Na principal coordena as tarefas; na câmera apenas informa o IP enquanto os servidores trabalham em suas tarefas. |

## CameraWebServer/app_httpd.cpp

| Função | Linha | O que faz |
|---|---|---|
| [rastroStreamStart](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L58) | 58 | Inicializa as métricas da sessão de streaming dentro da região crítica. |
| [rastroStreamFrame](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L62) | 62 | Atualiza contagem e tempos do último quadro transmitido. |
| [rastroSocketOpen](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L69) | 69 | Configura TCP_NODELAY em uma conexão aceita pelo servidor da câmera. |
| [ra_filter_init](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L88) | 88 | Prepara o filtro de média móvel das métricas do exemplo. |
| [ra_filter_run](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L102) | 102 | Atualiza a média de tempos usada no diagnóstico serial. |
| [enable_led](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L119) | 119 | Controla o LED de iluminação quando esse recurso está definido e configurado. |
| [bmp_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L131) | 131 | Rota auxiliar do exemplo para obter imagem no formato BMP. |
| [jpg_encode_stream](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L171) | 171 | Callback que entrega partes de uma conversão JPEG à resposta HTTP. |
| [capture_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L183) | 183 | Obtém um framebuffer, responde com JPEG e o devolve à câmera. |
| [stream_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L241) | 241 | Repete captura e envio de partes MJPEG e libera o buffer de cada quadro. |
| [parse_get](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L335) | 335 | Lê e aloca a consulta da URL usada pelos handlers de controle. |
| [cmd_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L356) | 356 | Escolhe qual parâmetro do sensor alterar a partir de var e val. |
| [print_reg](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L448) | 448 | Acrescenta valores de registradores do sensor ao diagnóstico. |
| [status_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L452) | 452 | Serializa ajustes e métricas da câmera em JSON. |
| [xclk_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L533) | 533 | Solicita mudança de clock do sensor e informa se foi aceita. |
| [reg_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L560) | 560 | Controle avançado de escrita em registrador do sensor, herdado do exemplo. |
| [greg_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L592) | 592 | Controle avançado de leitura de registrador do sensor. |
| [parse_get_var](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L622) | 622 | Auxilia a leitura de parâmetros das rotas avançadas. |
| [pll_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L630) | 630 | Controle avançado de PLL do sensor, herdado do exemplo. |
| [win_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L658) | 658 | Controle avançado da janela de leitura do sensor. |
| [index_handler](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L693) | 693 | Entrega a interface de controle da câmera compatível com o sensor. |
| [startCameraServer](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L711) | 711 | Registra as rotas HTTP e inicia servidor de controle e servidor de streaming. |
| [setupLedFlash](https://github.com/lyralucas21/rastro/blob/main/CameraWebServer/app_httpd.cpp#L884) | 884 | Configura o canal de controle do LED quando a pinagem permite. |

## CameraWebServer/board_config.h

Arquivo de configuração, mapeamento de pinos ou recurso auxiliar; a explicação está no guia e no próprio código.

## CameraWebServer/camera_pins.h

Arquivo de configuração, mapeamento de pinos ou recurso auxiliar; a explicação está no guia e no próprio código.

## CameraWebServer/config.h

Arquivo de configuração, mapeamento de pinos ou recurso auxiliar; a explicação está no guia e no próprio código.

## CameraWebServer/partitions.csv

Arquivo de configuração, mapeamento de pinos ou recurso auxiliar; a explicação está no guia e no próprio código.

## painel-javascript.txt

| Função | Linha | O que faz |
|---|---|---|
| [notice](painel-javascript.txt) | 7 | Mostra ou esconde um aviso no painel. |
| [request](painel-javascript.txt) | 8 | Faz uma consulta sem cache, com timeout, e rejeita status HTTP sem sucesso. |
| [openDb](painel-javascript.txt) | 9 | Abre/migra o IndexedDB versão 3 e trata bloqueio por outra aba ou troca de versão. |
| [savedPhotos](painel-javascript.txt) | 16 | Lê os álbuns armazenados no navegador. |
| [storePhoto](painel-javascript.txt) | 17 | Grava o álbum com um token de revisão e espera a confirmação da transação. |
| [eventKey](painel-javascript.txt) | 18 | Forma a identidade usando sessão e ID. |
| [key](painel-javascript.txt) | 19 | Usa a chave explícita do registro ou deriva sessão e ID. |
| [fixTime](painel-javascript.txt) | 21 | Estima a data de um evento da sessão atual quando o relógio se torna conhecido. |
| [ultimoHorario](painel-javascript.txt) | 22 | Calcula o horário do último sinal da ocorrência a partir dos tempos relativos. |
| [detalhesOcorrencia](painel-javascript.txt) | 23 | Produz o texto com detecções, último sinal, intervalo e estado do registro. |
| [allEvents](painel-javascript.txt) | 29 | Combina placa, arquivos e metadados das fotos e ordena por data/identidade. |
| [filtered](painel-javascript.txt) | 30 | Aplica busca, tipo, fotos e intervalo de datas. |
| [historySlice](painel-javascript.txt) | 33 | Seleciona 25 registros e mantém a leitura ancorada quando surgem eventos novos. |
| [moveHistory](painel-javascript.txt) | 47 | Avança ou recua a posição no histórico e redesenha a tabela. |
| [thumbnail](painel-javascript.txt) | 50 | Mantém uma Object URL para a miniatura do álbum. |
| [render](painel-javascript.txt) | 51 | Atualiza armazenamento, resumo, paginação e tabela, evitando redesenho desnecessário. |
| [framesOf](painel-javascript.txt) | 59 | Lê a lista de quadros e mantém compatibilidade com o formato antigo de uma foto. |
| [albumSize](painel-javascript.txt) | 60 | Conta fotos de um álbum. |
| [totalPhotos](painel-javascript.txt) | 61 | Conta fotos em todos os álbuns, não apenas quantos álbuns existem. |
| [renderStorage](painel-javascript.txt) | 62 | Mostra limites e falhas de armazenamento e explica por que a captura pode estar pausada. |
| [syncAlbumMetadata](painel-javascript.txt) | 70 | Atualiza os metadados de uma ocorrência encerrada sem substituir suas imagens. |
| [galleryRender](painel-javascript.txt) | 79 | Seleciona e exibe a foto corrente e configura seus botões e download. |
| [showPhoto](painel-javascript.txt) | 90 | Abre um álbum a partir da primeira foto e interrompe a reprodução anterior. |
| [eligibleAlbum](painel-javascript.txt) | 99 | Verifica se uma ocorrência aberta está elegível a continuar sendo fotografada. |
| [scheduleAlbum](painel-javascript.txt) | 100 | Decide se deve iniciar uma captura automática respeitando intervalo, visibilidade e limites. |
| [capture](painel-javascript.txt) | 115 | Reaproveita quadro recente ou pede JPEG, grava o álbum e trata falhas e retorno do vídeo. |
| [setStatus](painel-javascript.txt) | 144 | Atualiza estados, controles e saúde a partir da resposta da placa. |
| [command](painel-javascript.txt) | 145 | Envia modo/pausa/sirene e evita ações concorrentes durante a confirmação. |
| [poll](painel-javascript.txt) | 147 | Busca status e histórico quando necessário e agenda a próxima consulta sem sobreposição. |
| [cameraState](painel-javascript.txt) | 157 | Atualiza o estado e a mensagem da câmera. |
| [stopCamera](painel-javascript.txt) | 161 | Cancela a conexão corrente e invalida respostas de gerações antigas. |
| [joinBytes](painel-javascript.txt) | 165 | Une blocos de bytes recebidos em partes pela rede. |
| [displayCameraFrame](painel-javascript.txt) | 194 | Decodifica um JPEG, verifica a geração, exibe, libera URL antiga e mede FPS de exibição. |
| [cameraConnect](painel-javascript.txt) | 210 | Consulta a câmera, escolhe streaming ou fotos em sequência e administra timeouts/reconexão. |
| [backupRevision](painel-javascript.txt) | 333 | Cria um identificador de revisão para detectar alterações concorrentes entre abas. |
| [backupMessage](painel-javascript.txt) | 334 | Mostra resultado ou erro do fluxo de backup. |
| [backupButtons](painel-javascript.txt) | 335 | Bloqueia controles enquanto uma operação de backup está em andamento. |
| [savedArchives](painel-javascript.txt) | 336 | Lê os registros arquivados/importados no navegador. |
| [backupEvent](painel-javascript.txt) | 337 | Valida e normaliza metadados de evento aceitos no arquivo de backup. |
| [jpegBackupCheck](painel-javascript.txt) | 350 | Verifica estrutura e dimensões do JPEG, além de limites e marcadores necessários. |
| [blobDataUrl](painel-javascript.txt) | 369 | Converte os bytes do JPEG para uma representação textual data URL. |
| [backupFrame](painel-javascript.txt) | 370 | Valida um quadro e calcula sua identidade para deduplicação; pode conferir a decodificação. |
| [buildBackup](painel-javascript.txt) | 380 | Reúne eventos e álbuns disponíveis no JSON com limites de quantidade e tamanho. |
| [parseBackup](painel-javascript.txt) | 396 | Lê e valida completamente o arquivo antes de planejar alterações. |
| [planBackup](painel-javascript.txt) | 421 | Calcula acréscimos e duplicados e verifica se o conjunto resultante respeita os limites. |
| [commitBackup](painel-javascript.txt) | 437 | Revalida concorrência e confirma fotos e arquivos em uma transação conjunta. |
| [restoreBackup](painel-javascript.txt) | 463 | Relê dados atuais, planeja, confirma e atualiza a interface após sucesso. |
| [stopGallery](painel-javascript.txt) | 497 | Interrompe a reprodução automática de fotos. |
| [releaseGalleryThumbs](painel-javascript.txt) | 498 | Libera URLs temporárias de miniaturas da janela. |
| [eventInterval](painel-javascript.txt) | 499 | Calcula o intervalo entre primeiro e último sinal quando existem tempos utilizáveis. |
| [eventDate](painel-javascript.txt) | 500 | Aceita uma data utilizável para filtros e gráficos. |
| [inDateRange](painel-javascript.txt) | 501 | Aplica as datas inicial/final inclusivas e exclui horários desconhecidos quando há filtro de data. |
| [showOccurrence](painel-javascript.txt) | 502 | Abre detalhes mesmo sem imagens salvas. |
| [galleryDetails](painel-javascript.txt) | 505 | Combina metadados atuais, miniaturas, intervalos e o estado sem fotos. |
| [scheduleGallery](painel-javascript.txt) | 521 | Agenda o avanço de foto, interrompendo no final ou quando a janela/aba deixa de estar disponível. |
| [renderActivity](painel-javascript.txt) | 532 | Calcula indicadores e gráfico dos registros filtrados e cria filtros por dia. |
| [cameraHealth](painel-javascript.txt) | 547 | Mostra o diagnóstico textual da câmera na área de saúde. |
| [renderNight](painel-javascript.txt) | 551 | Mostra a configuração real sem sobrescrever uma edição ainda não salva e bloqueia ações concorrentes. |
| [init](painel-javascript.txt) | 568 | Abre o banco, lê dados salvos, inicia câmera e polling, ou carrega a demonstração quando solicitada. |

## painel-estilos.css

Arquivo de configuração, mapeamento de pinos ou recurso auxiliar; a explicação está no guia e no próprio código.

## Recursos que não são lógica de detecção

`pagina.h` reúne HTML, CSS e JavaScript. `camera_index.h` contém a interface comprimida herdada do exemplo da câmera; os vetores de bytes não precisam ser decorados. `docs/esquematico.svg` é o desenho da montagem. `gabinete/` contém arquivos de modelagem e geração da caixa; não roda nas placas. Os documentos de etapas são históricos de desenvolvimento.

## Manipuladores e classes do JavaScript

`CameraFrames` tem um construtor e o método `push`, que recompõem os JPEGs do multipart. Além das funções nomeadas, há callbacks de clique, teclado, mudança de campo, visibilidade, IndexedDB e rede. Procure por `.onclick`, `addEventListener`, `onsuccess`, `oncomplete`, `onabort` e `setTimeout` no arquivo de leitura. Esses callbacks são explicados nos capítulos de interface, vídeo, fotos e backup.

## Constantes que você deve localizar

| Constante/valor | Papel |
|---|---|
| AQUECIMENTO = 30000 | Calibração inicial do PIR |
| INTERVALO = 10000 | Separação entre ocorrências |
| LIMITE = 100 | Eventos retidos na principal |
| MAX_ALBUM = 5 | Fotos por ocorrência |
| MAX_PHOTOS = 300 | Fotos no navegador |
| PHOTO_INTERVAL = 4000 | Intervalo mínimo entre tentativas de fotos |
| HISTORY_PAGE_SIZE = 25 | Linhas desenhadas por página |
| CAMERA_VIDEO_RETRY_MS = 60000 | Janela para tentar retornar ao vídeo |
| BACKUP_VERSION = 1 | Versão do formato de arquivo JSON |
| IndexedDB versão 3 | Esquema do banco local, distinto da versão do JSON |
| ProgramacaoNoturna::TAMANHO = 20 | Bytes da configuração noturna |