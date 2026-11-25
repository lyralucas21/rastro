# Roteiro para apresentar o RASTRO

Texto de apoio da versão de 15/11/2026. Adapte a fala ao tempo disponível e mostre apenas funções que puder demonstrar. Os exemplos de fala abaixo podem ser ensaiados; não é preciso ler tudo na hora.

## Abertura em 30 segundos

“O RASTRO é uma instalação interativa sobre vigilância. A ideia é tornar visível o registro de um movimento: o sensor detecta atividade, o objeto reage e um painel mostra o acontecimento com horário e imagens. O projeto usa duas placas ESP32 e um navegador. Uma placa organiza os eventos e controla o objeto; a outra fornece as imagens.”

“Nós registramos detecções e ocorrências de movimento. Isso não é uma identificação ou contagem exata de pessoas.”

## Apresentação de aproximadamente 3 minutos

### 0:00–0:35 — ideia e experiência

“Normalmente somos observados por sistemas cujo funcionamento não vemos. O RASTRO expõe esse processo: quem passa pode perceber a reação do objeto e encontrar o registro no painel. O objetivo não é esconder a coleta, mas colocá-la em discussão.”

Mostre o objeto e a tela. Explique que é um protótipo de instalação interativa.

### 0:35–1:15 — componentes e fluxo

“O PIR fornece um sinal digital para a principal, no GPIO 14. A principal controla o LCD por I2C, aciona o buzzer no GPIO 15 e atende o painel pela rede. A ESP32-CAM fornece as imagens diretamente ao navegador. O vídeo não precisa passar inteiro pela principal.”

“O código da placa foi feito em C++ para Arduino. O painel usa HTML, CSS e JavaScript. A página fica guardada no próprio firmware, e é executada pelo navegador quando a abrimos.”

### 1:15–1:55 — detecções e agrupamento

“Um movimento prolongado não deve produzir uma nova ocorrência a cada volta do programa. Por isso agrupamos sinais próximos. Uma nova subida do sinal pode aumentar o número de detecções da mesma ocorrência; o agrupamento termina depois de aproximadamente 10 segundos sem atividade.”

Faça um movimento curto e espere. Mostre uma ocorrência e seu número de detecções. Não diga que uma linha é necessariamente uma pessoa diferente.

### 1:55–2:30 — imagens e armazenamento

“A principal preserva os últimos 100 eventos e os contadores. As fotos ficam no navegador. Cada ocorrência pode receber até cinco fotos, com intervalo mínimo de quatro segundos, enquanto o painel está aberto e visível e as condições permitem. Podemos abrir a sequência e exportar um backup.”

“A sequência de fotos não é uma gravação contínua de vídeo.”

### 2:30–3:00 — modos e conclusão

“Monitoramento registra sem som; Apresentação dá um toque curto; Ausente usa uma sirene de até oito segundos por nova ocorrência. Acrescentamos uma programação diária, atualmente das 22h às 7h, executada na própria placa. Fechar o navegador não interrompe essa troca de modo, mas interrompe a possibilidade de fotos automáticas pela página.”

“O que queremos mostrar é que transformar um movimento em um registro envolve decisões de detecção, agrupamento, armazenamento e apresentação.”

## Versão de 7 a 10 minutos

Use a versão curta como estrutura e acrescente os blocos abaixo.

### Arquitetura — mais 1 minuto

Desenhe três caixas: principal, câmera e navegador. Ligue principal e navegador por HTTP. Ligue câmera e navegador por vídeo/foto. Ligue PIR, LCD e buzzer somente à principal no desenho lógico. Mostre uma quarta caixa pequena para o servidor de horário NTP.

Explique: “A principal serve dados de evento em JSON. A câmera serve JPEGs. O navegador combina os dois. Separar essas responsabilidades facilita entender onde cada problema acontece.”

### Lógica do código — mais 1 minuto

Abra `setup()` e `loop()` da principal.

“O setup prepara a placa e recupera o que foi salvo. O loop atende rede, verifica horário, lê o sensor, atualiza a ocorrência, controla a duração do som e salva alterações. A sirene não usa uma espera bloqueante de oito segundos; o programa compara o tempo decorrido para continuar atendendo outras tarefas.”

Mostre `ocorrencia.h`. Explique os estados `Inicio`, `Atualizacao`, `Fim` e `Nenhuma` com um pequeno desenho de sinais.

### Persistência — mais 1 minuto

“Os eventos de trabalho ficam em RAM e são codificados em registros de 64 bytes para a NVS. Cada registro tem versão e CRC, porque não seria correto salvar ponteiros da estrutura de RAM. Há uma partição de dados dedicada de 64 KB. O histórico é circular: ao entrar um novo evento depois do limite, o mais antigo sai.”

Explique a janela de salvamento: atualizações pendentes podem se perder em um corte de energia. Não prometa gravação instantânea de cada amostra.

### Painel e vídeo — mais 1 minuto

“O JavaScript consulta o estado e só busca o histórico de novo quando necessário. O vídeo usa MJPEG: uma sequência de JPEGs dentro de uma resposta HTTP. O painel reconstrói os quadros mesmo quando a rede divide os dados em vários pacotes.”

“Escolhemos resolução e compressão para buscar mais continuidade. FPS varia com exposição, tamanho dos quadros, rede e processamento. Um número alto em uma cena preta não comprova o mesmo desempenho em uma sala detalhada.”

### Programação e limites — mais 1 minuto

“O modo noturno compara minutos do dia. Como 22h–7h cruza a meia-noite, a condição é estar depois das 22h OU antes das 7h. A programação respeita a pausa e permite uma escolha manual até a próxima transição.”

“O programa depende de hora sincronizada após ligar. Ele não cria visão infravermelha. Também não faz reconhecimento facial nem notificação de celular nesta versão.”

## Demonstração segura e organizada

1. Confira o IP correto, a rede, a câmera e a iluminação antes de começar.
2. Aguarde terminar a calibração do PIR.
3. Use Apresentação para o toque curto, se for adequado ao ambiente. A escolha manual não desativa permanentemente a programação noturna.
4. Faça um movimento e mostre como a ocorrência aparece.
5. Aguarde o encerramento do agrupamento; aproveite a espera para explicar por que não há uma ocorrência por ciclo do programa.
6. Abra uma sequência já existente para não depender de capturar cinco fotos ao vivo.
7. Mostre busca, datas, resumo e backup.
8. Mostre a configuração noturna e explique que a troca roda na placa.
9. Termine no modo apropriado ao local. Não deixe um horário temporário de teste salvo por engano.

Não apague dados nem troque redes durante a apresentação se isso não fizer parte de uma demonstração planejada.

## Se algo falhar na hora

**Sem imagem:** separe a falha da câmera do registro da principal. Mostre o histórico e explique que são caminhos distintos. Confira o aviso da câmera e se há outra transmissão aberta. Uma demonstração de fotos já salvas pode continuar, sem fingir que são imagens ao vivo.

**Sem data:** explique a dependência de sincronização NTP. Não invente um horário. Mostre o estado do relógio.

**Sem foto nova:** confira se a aba está visível, se a captura automática está ligada, se a câmera responde e se o limite de fotos foi atingido. Um evento sem foto ainda é um evento válido do PIR.

**Sem som:** confira modo e pausa. Monitoramento é silencioso por definição. Silêncio não prova que o sensor falhou.

**Contagem inesperada:** mostre a diferença entre detecções, ocorrências e eventos de sistema. Evite tentar justificar números como se fossem pessoas identificadas.

## Respostas curtas para a banca

**O diferencial técnico?** Integração de detecção, agrupamento temporal, objeto físico, imagem, interface e persistência, com divisão clara entre duas placas e navegador.

**Por que agrupar?** Para representar um período de atividade em vez de inflar o histórico com uma linha a cada repetição da leitura.

**O que roda sem site aberto?** Sensor, agrupamento, som, LCD, histórico e programação de modo. Fotos automáticas da página exigem o navegador aberto e visível.

**Onde está o banco de dados?** Há dois armazenamentos diferentes: NVS na placa para eventos/preferências e IndexedDB no navegador para fotos/arquivos.

**O que acontece se trocar o IP?** A placa continua com seus dados, mas o navegador trata o novo endereço como outra origem; fotos anteriores precisam ser transportadas por backup quando necessário.

**Como evitar que a sirene trave tudo?** Comparando o tempo decorrido com `millis()` em vez de usar um `delay()` longo para manter o som.

**O noturno foi testado?** Sim, a mudança foi verificada numa virada real de horário temporário e a configuração final foi conferida depois de reiniciar a placa.

**Qual melhoria futura?** Alertas remotos, análise de regiões na imagem ou outro hardware de câmera podem ser próximos passos. Apresente como possibilidades, não como funções prontas.

## Checklist de entendimento pessoal

Você deve conseguir apontar no código:

- Onde são definidos GPIO 14, GPIO 15, SDA 21 e SCL 22.
- Onde estão `setup()` e `loop()` de cada placa.
- Onde uma ocorrência começa e termina.
- Onde o modo determina a duração do som.
- Onde o horário noturno é comparado e salvo.
- Onde a principal entrega `/status` e `/eventos`.
- Onde a câmera entrega `/capture` e `/stream`.
- Onde o painel decide salvar uma foto.
- Onde ficam os limites de 100 eventos, 5 fotos por ocorrência e 300 fotos.
- Onde são criados e restaurados os backups.

Se souber explicar essas dez localizações sem decorar a redação do guia, terá uma base sólida para a apresentação.
