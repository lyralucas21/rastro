# RASTRO: da ideia ao código, para entender e apresentar

Guia da versão de 15/11/2026. Leia junto do código incluído neste pacote. A explicação descreve o que foi implementado; possibilidades futuras aparecem como futuras, não como funções prontas.

## 1. A primeira explicação que você precisa dominar

O RASTRO é uma instalação interativa que torna visível o ato de registrar movimentos em um ambiente. Um sensor fornece um sinal de movimento, uma ESP32 organiza os acontecimentos e controla som e LCD, outra ESP32 fornece imagens, e o navegador reúne tudo em um painel.

Existem **três programas trabalhando em lugares diferentes**:

1. O firmware em C++ da ESP32 principal: sensor, agrupamento, sirene, LCD, relógio, histórico, modos e programação noturna.
2. O firmware em C++ da ESP32-CAM: inicialização da câmera, captura JPEG e servidores de imagem.
3. O painel em HTML, CSS e JavaScript: interface, consultas às placas, exibição do vídeo, captura de fotos, álbuns e backup.

O Arduino IDE é a ferramenta usada para compilar e gravar os dois firmwares. O navegador executa JavaScript; a ESP32 principal guarda o texto da página e o entrega pela rede. Ela não executa o JavaScript do painel.

O ponto mais importante para apresentar corretamente: **o PIR detecta variações de movimento compatíveis com seu funcionamento; ele não identifica uma pessoa nem garante uma contagem de pessoas**. Uma pessoa pode gerar várias detecções. Várias pessoas próximas podem ser agrupadas em uma ocorrência.

## 2. O caminho completo de um acontecimento

```text
Movimento no ambiente
    ↓
PIR: sinal elétrico na saída OUT
    ↓ GPIO 14
ESP32 principal
    ├─ valida: registro ligado? calibração terminou?
    ├─ cria ou atualiza uma ocorrência
    ├─ atualiza contadores e histórico
    ├─ controla o buzzer conforme o modo
    ├─ envia mensagem à tarefa do LCD
    └─ responde ao painel por HTTP
                    ↓
             Navegador aberto
                    ├─ atualiza números e histórico
                    ├─ recebe imagens diretamente da ESP32-CAM
                    └─ salva fotos no IndexedDB deste navegador
```

A principal e a câmera usam a mesma rede local. No funcionamento normal, a principal **não recebe todo o vídeo para depois retransmiti-lo**. O navegador conversa com a principal para obter os eventos e com a câmera para obter as imagens. Isso poupa processamento e tráfego na placa principal.

As ligações TX/RX usadas durante a gravação da câmera são uma ponte de programação. Elas não são o caminho normal do vídeo. Alimentação e GND são outra questão: a montagem física pode compartilhar a alimentação; a comunicação de funcionamento entre os programas ocorre pela rede.

## 3. Os componentes e os pinos

| Componente | Papel no sistema | Ligação usada no código da principal |
|---|---|---|
| PIR | Fornecer um sinal digital de movimento | GPIO 14 |
| Buzzer ativo | Emitir som quando acionado | GPIO 15 |
| LCD 16×2 com I2C | Mostrar estado e IP | SDA 21, SCL 22, endereço 0x27 |
| Wi-Fi da principal | Servir painel e APIs | Biblioteca WiFi |
| ESP32-CAM | Capturar e servir imagens | Pinagem AI-Thinker em arquivo próprio |

HIGH e LOW são níveis lógicos. `digitalRead(PIN_PIR)` lê um desses níveis. `digitalWrite(PIN_BUZZER, HIGH)` aciona a saída usada pelo buzzer ativo; LOW a desliga. Isso não é síntese de uma música: o buzzer ativo produz seu som quando alimentado pelo acionamento correspondente.

I2C permite conversar com o módulo do LCD usando dois sinais de comunicação, SDA e SCL. `0x27` é o endereço procurado na montagem atual. Embora um comentário antigo mencione a possibilidade de 0x3F em outros módulos, **o programa atual inicializa e verifica 0x27**.

Na câmera, os nomes `Y2_GPIO_NUM`, `XCLK_GPIO_NUM`, `VSYNC_GPIO_NUM` etc. são definidos por `camera_pins.h`. A seleção `CAMERA_MODEL_AI_THINKER` escolhe esse conjunto de pinos; ela não contradiz usar **ESP32 Dev Module** no menu do Arduino IDE. Uma escolha define a pinagem no código, a outra define o alvo de compilação.

## 4. Vocabulário mínimo de C++ para ler os arquivos

```cpp
const int PIN_PIR = 14;
bool armado = true;
uint32_t totalMovimentos = 0;
```

`const` indica um valor que não deve mudar por atribuição normal. `int` é um tipo inteiro. `bool` representa verdadeiro ou falso. `uint32_t` é um inteiro sem sinal de 32 bits. O `=` inicializa ou atribui; `==` compara. `++contador` soma um. `!condicao` inverte o resultado lógico. `&&` exige que duas condições sejam verdadeiras; `||` aceita uma ou outra.

Uma função agrupa uma tarefa: recebe parâmetros, executa comandos e pode devolver um resultado. `void` significa que não devolve um valor. `return` encerra a função. Uma referência como `Evento &e` dá acesso ao próprio objeto, não a uma cópia independente; alterar `e` altera o evento referido.

`struct` reúne dados e funções relacionados. `enum class` cria opções nomeadas e evita confundi-las com números quaisquer. `for` repete um bloco. `if` escolhe um caminho. O operador `?:` é uma escolha curta: `armado ? "true" : "false"` produz um dos textos.

`#include` inclui declarações de outro arquivo durante a compilação. `#define` cria uma substituição do pré-processador. `#pragma once` evita a inclusão repetida de um cabeçalho no mesmo arquivo de compilação. Arquivos `.h` ajudam a dividir o sistema; não são programas que você grava separadamente.

`String` é a classe de texto do Arduino. `char[]` é um vetor de caracteres, geralmente terminado por um byte zero. `c_str()` permite acessar o texto como ponteiro para caracteres. Um ponteiro guarda um endereço de memória. Não é uma cópia do conteúdo apontado.

## 5. `setup()` e `loop()` da principal

`setup()` roda uma vez após ligar ou reiniciar. Na versão atual, ele:

1. Inicia a comunicação serial em 115200 e configura os pinos.
2. Coloca o buzzer em LOW.
3. Cria a tarefa e a fila do LCD.
4. Gera um identificador de sessão com números aleatórios.
5. Recupera histórico, modo, pausa e programação noturna.
6. Configura Wi-Fi, lista redes para diagnóstico e inicia a conexão.
7. Registra as rotas HTTP `/`, `/status`, `/eventos` e `/cmd`.
8. Inicia o servidor e registra um evento de inicialização.

`loop()` roda repetidamente. Ele atende clientes HTTP, verifica a programação noturna aproximadamente a cada segundo, procura a câmera, lê o PIR, atualiza o agrupamento, controla a duração do som, acompanha o Wi-Fi, publica mensagens ao LCD, corrige datas e salva alterações do histórico.

O pequeno `delay(2)` ao final dá uma pausa curta ao ciclo. Não existe um `delay(8000)` para manter a sirene ligada. Esse detalhe importa: uma espera longa bloquearia outras tarefas da lógica principal.

### O papel de `millis()`

`millis()` informa o tempo desde a inicialização em milissegundos. É adequado para intervalos, mas não informa a data real. O código usa comparações como:

```cpp
if (uint32_t(agora - ultimaProgramacao) >= 1000) {
    ultimaProgramacao = agora;
    aplicarNoturno();
}
```

A subtração sem sinal mantém a comparação de intervalos funcionando ao redor do retorno do contador de 32 bits, desde que se respeite a escala prevista para o intervalo. Ela evita precisar parar o programa para esperar um segundo.

### Calibração inicial

Durante os primeiros 30 segundos de execução, a leitura do PIR não é tratada como atividade válida. O estado apresentado é “Calibrando”. Isso reduz a interpretação de sinais da inicialização como ocorrências reais. O código usa `AQUECIMENTO = 30000` e verifica esse tempo antes de considerar o sinal ativo.

## 6. Detecção, ocorrência e evento: três coisas diferentes

**Detecção**: no agrupador, uma nova subida do sinal considerado ativo, de falso para verdadeiro. Um sinal mantido alto durante vários ciclos não adiciona uma detecção em cada ciclo.

**Ocorrência**: um agrupamento temporal de atividade. Ela começa quando aparece atividade e fica aberta enquanto há atividade ou enquanto ainda não se completou o intervalo de encerramento. A separação usada é 10 segundos desde a última amostra ativa.

**Evento**: qualquer registro do histórico. Além de movimento, há inicialização, pausa, retomada, mudança de modo e acionamento/desligamento de sirene. Por isso 100 eventos não significam 100 ocorrências de movimento.

### Exemplo concreto

```text
t = 0 s   sinal sobe: ocorrência A começa; detecções de A = 1
t = 2 s   sinal desce
t = 5 s   sinal sobe: continua A; detecções de A = 2
t = 7 s   sinal desce; última amostra ativa ficou próxima desse instante
t ≈ 17 s  sem nova atividade: A encerra após o intervalo de 10 s
t = 25 s  novo sinal: começa ocorrência B
```

O instante exato da última amostra depende do ciclo do programa. O PIR físico também pode manter seu sinal alto por um tempo configurado no módulo. Portanto, duração do sinal não equivale automaticamente ao tempo que uma pessoa ficou no local.

### Como ler `ocorrencia.h`

`AgrupadorOcorrencia` guarda `aberta`, `anterior`, `inicio`, `ultimo`, `publicado` e `deteccoes`. `atualizar(agora, ativo)` devolve uma destas mudanças: `Nenhuma`, `Inicio`, `Atualizacao` ou `Fim`.

- Se não havia ocorrência e chega atividade, inicia uma.
- Se chega uma nova subida dentro da ocorrência, incrementa detecções.
- Enquanto o sinal permanece ativo, atualiza `ultimo`; publica atualizações em intervalos ou nas mudanças relevantes.
- Quando o sinal não está ativo e a lacuna chega a 10 s, encerra o agrupamento.
- Se uma amostra ativa chega depois de uma lacuna suficiente, ela pode iniciar um agrupamento novo.

Na principal, `atualizarOcorrencia()` transforma essas mudanças em alterações de `Evento`. `slotOcorrencia` aponta para a posição do evento corrente e `idOcorrencia` ajuda a confirmar que essa posição ainda contém o evento esperado.

`totalMovimentos` conta subidas válidas do sinal. `totalOcorrencias` aumenta quando `registrar("movimento")` cria uma ocorrência. Esses totais podem ser diferentes e não devem ser chamados de número de pessoas.

## 7. Histórico circular: como cabem os registros

O vetor `eventos[LIMITE]` tem 100 posições. `proximo` aponta para a próxima posição de gravação e `quantidade` informa quantas posições têm registros úteis. O avanço usa:

```cpp
proximo = (proximo + 1) % LIMITE;
```

O operador `%` obtém o resto da divisão. Depois da posição 99, o resultado volta a zero. Quando o vetor enche, novos eventos substituem os mais antigos. Isso é um **buffer circular**.

O contador acumulado de ocorrências é preservado separadamente nos registros persistentes; ele pode continuar maior que a quantidade de ocorrências ainda visíveis nos últimos 100 eventos.

`Evento` guarda ID, tempo desde a inicialização, ordem, data textual, tipo, último tempo de movimento, número de detecções, estado aberto/fechado, sessão de origem, horário Unix, indicação de interrupção e posição de armazenamento.

Uma sessão distingue uma execução da placa de outra. A chave usada nas fotos combina a sessão com o ID, evitando associar a foto de um evento antigo a um evento de outra execução que tenha um número parecido.

## 8. RAM, flash, NVS, PSRAM e navegador

| Local | O que significa | Uso no projeto |
|---|---|---|
| RAM da principal | Memória de trabalho volátil | Vetor de eventos, estados, textos temporários, rede |
| Flash | Armazenamento não volátil | Programa, página e partições de dados |
| NVS | Organização de dados persistentes na flash | Histórico, modo, pausa e programação noturna |
| PSRAM da câmera | RAM adicional da placa de câmera | Buffers de imagem |
| IndexedDB do navegador | Banco local do navegador para essa origem | Fotos e registros arquivados/importados |

Desligar a placa apaga a RAM, mas não deve apagar a NVS em uma atualização normal. Fechar o navegador não deve apagar seu IndexedDB, embora limpeza de dados, modo privado, troca de perfil e políticas de armazenamento possam afetá-lo. As fotos não são gravadas automaticamente na flash da principal.

### `historico.h`: formato persistente

Um ponteiro como `const char* tipo` só faz sentido dentro do programa em execução. Por isso o código não copia a estrutura de RAM diretamente para a flash. Ele converte cada evento para um registro fixo de **64 bytes**, sem ponteiros.

O registro contém uma assinatura, geração de gravação, ordem, contadores, dados do evento, sessão, código do tipo, flags, versão e CRC. `codificar()` prepara esses bytes; `decodificar()` valida e reconstrói um evento.

**CRC é uma verificação de integridade**, não criptografia. Ajuda a rejeitar um registro alterado ou incompleto; não protege o conteúdo contra uma pessoa que consiga lê-lo.

### `persistencia.h`: quando salvar

`marcarHistorico()` indica posições alteradas. `salvarHistorico()` reúne as alterações em vez de gravar a flash em toda leitura do sensor. Criação e encerramento usam a janela curta de até cerca de 2 s entre oportunidades de gravação; uma ocorrência ativa sem urgência usa checkpoints de cerca de 30 s.

Isso reduz gravações repetidas, mas significa que um corte de energia pode perder atualizações ainda pendentes. Persistência não é uma promessa de que todo último milissegundo foi gravado. O painel informa quando há salvamento pendente.

Ao iniciar, `carregarHistorico()` lê os registros válidos, ordena por ordem, recupera contadores, prepara posições livres e encerra como interrompidas as ocorrências que ficaram abertas por causa de um reinício.

### `migracao_memoria.h` e `historico_inicial.h`

A migração copia dados antigos para a área dedicada e grava o marcador de conclusão por último. Uma execução interrompida pode retomar a migração. O arquivo de histórico inicial foi usado na instalação para importar registros anteriores.

**Nesta cópia para compartilhar, o histórico inicial está vazio.** O Lucas recebe o programa, não os registros pessoais de uso da instalação do Arthur. Isso não apaga os dados da placa do Arthur, que não foi alterada por este empacotamento.

## 9. As partições da principal

`partitions.csv` precisa acompanhar o sketch. Ele descreve regiões diferentes dentro da flash de 4 MB:

| Região | Endereço inicial | Tamanho | Finalidade |
|---|---|---|---|
| nvs | 0x9000 | 0x5000 | NVS padrão |
| otadata | 0xE000 | 0x2000 | Metadados reservados para o esquema de aplicações |
| app0 | 0x10000 | 0x300000 | Programa de até 3 MB |
| spiffs | 0x310000 | 0xD0000 | Região de sistema de arquivos reservada |
| rastro_data | 0x3E0000 | 0x10000 | 64 KB dedicados ao histórico e preferências |
| coredump | 0x3F0000 | 0x10000 | Região reservada para diagnóstico |

A existência de `spiffs` não significa que o código esteja salvando as fotos ali. A existência de `otadata` também não significa que foi implementada atualização remota: esta versão usa uma aplicação grande e não oferece uma rota OTA no painel.

Na atualização feita no computador do Arthur, foi gravado somente o programa em 0x10000. Para reproduzir no Arduino IDE, mantenha a tabela correta e evite “Erase All Flash” se quiser preservar os dados. O programa testa a abertura da partição `rastro_data` e do namespace `rastro-h2`.

## 10. Modos e sirene sem travar o programa

`modos.h` define as opções e o controle temporal do alarme:

| Modo | Registro quando ligado | Som por nova ocorrência |
|---|---|---|
| Monitoramento | Continua | Nenhum |
| Ausente | Continua | Até 8 s |
| Apresentação | Continua | Toque de 250 ms |

Fotos continuam dependendo do painel e da opção de captura, em qualquer modo. Escolher um modo não é o mesmo que ligar ou pausar o registro.

`ControleSirene` guarda se está ativa, o instante de início e a duração. `movimento()` só dispara quando há uma ocorrência nova, o registro está ligado e o modo permite som. `atualizar()` compara o tempo decorrido e desliga quando necessário. `manual()` permite o teste nos modos com som. `parar()` cancela o som.

Mudar de modo chama `silenciar()`. Pausar o registro encerra a ocorrência corrente, silencia e limpa o estado de movimento usado pela lógica. O código tenta salvar o modo e a pausa antes de confirmar a mudança. Se não consegue, responde com erro em vez de fingir que a escolha foi salva.

`OperacaoSalva` compacta modo e pausa em um byte, com bits de identificação e validação. O uso de bits aqui economiza espaço e permite conferir se o valor recuperado tem um formato reconhecido.

## 11. Programação noturna, explicada inteira

A programação não é um temporizador no JavaScript. Ela roda na principal. O navegador serve para editar e consultar.

O padrão sugerido é 22:00 a 07:00. No intervalo usa Ausente; fora usa Monitoramento ou Apresentação, conforme a escolha. Na placa do Arthur, a programação foi ativada em 22:00–07:00, com Monitoramento no restante do dia. Em uma placa nova, a configuração começa desativada até ser salva/ativada pelo usuário.

### Convertendo horários

22:00 corresponde a `22 * 60 = 1320` minutos do dia. 07:00 corresponde a 420. O relógio Unix é convertido para minutos locais usando UTC−3, coerente com o horário de Brasília configurado na placa. O resto por 1440 informa o minuto dentro do dia.

Para um intervalo que não atravessa a meia-noite, como 08:00–10:00, a condição é:

```cpp
minuto >= inicio && minuto < fim
```

Para 22:00–07:00, que atravessa a meia-noite, a condição é:

```cpp
minuto >= inicio || minuto < fim
```

O início é incluído; o fim é excluído. Horários iguais são rejeitados para não haver ambiguidade sobre zero horas ou 24 horas de funcionamento.

### Quem faz cada parte

`ProgramacaoNoturna` guarda ativação, início, fim, modo diurno e suspensão por escolha manual. `valida()` confere limites. `noite()` calcula se o instante está no intervalo. `alvo()` devolve o modo desejado. `periodo()` identifica a transição mais recente em minutos absolutos; isso impede tratar meia-noite como se fosse o fim de uma faixa de 22h a 7h.

`relogioNoturno()` verifica se existe data sincronizada. `guardarNoturno()` grava a configuração de 20 bytes com versão e CRC. `carregarNoturno()` recupera e valida. `aplicarNoturno()` decide se deve mudar o modo e salva a operação antes de aplicar. Essa função é chamada aproximadamente uma vez por segundo no `loop()` e ao salvar os horários.

### Escolha manual e pausa

Se você escolhe Monitoramento manualmente às 23h, a escolha vale até a próxima transição, às 7h. Ela não é imediatamente anulada por uma verificação automática a cada segundo. A identidade da faixa é salva, portanto um reinício dentro da mesma faixa não deve cancelar esse ajuste manual.

Se o relógio ainda não está pronto, um marcador especial conserva a intenção manual e é associado ao período quando o horário se torna conhecido. A pausa do registro continua sendo respeitada: a programação altera modo, não força `armado = true`.

Desativar a programação interrompe as trocas automáticas e mantém o modo atual. Se a intenção é ficar sem som imediatamente, selecione Monitoramento. Salvar novamente a programação cancela a suspensão manual e reaplica os horários.

### Dependência do relógio

A principal solicita hora por NTP depois de conectar ao Wi-Fi. Sem hora válida, a programação mantém o modo corrente e informa espera. Se a rede cai com a placa ainda alimentada, o relógio pode continuar avançando. Após desligar a alimentação, é necessário sincronizar de novo; não foi instalado um relógio de bateria independente neste projeto.

A troca é verificada pelo ciclo do programa: o fato de o relógio cruzar exatamente um minuto não significa uma mudança com latência zero no mesmo instante da resposta HTTP. O teste na placa aguardou o modo efetivamente trocar, respeitando o ciclo aproximado de um segundo.

## 12. LCD em uma tarefa separada

A biblioteca do LCD e o barramento I2C podem demorar ou falhar. A lógica de rede e do sensor não deve ficar esperando cada atualização visual. Por isso há uma tarefa do FreeRTOS chamada `rastro-lcd`.

`MensagemLCD` contém duas linhas de 16 caracteres, mais o terminador de texto. `iniciarLCD()` cria uma fila de capacidade 1 e uma tarefa com pilha de 4096 bytes. `publicarLCD()` prepara uma cópia das duas linhas, evita repetir conteúdo igual e substitui a mensagem pendente pela mais recente.

`tarefaLCD()` recebe a cópia, verifica o endereço I2C, inicializa o display quando possível, escreve as linhas alteradas e atualiza a disponibilidade. Se o display falha, a tarefa volta a tentar. O estado é compartilhado por `std::atomic<bool>`, adequado para comunicar essa informação entre as tarefas sem uma leitura/escrita comum concorrente.

Isso não transforma todo o programa em vários programas independentes: a lógica de eventos continua no ciclo principal. A separação é específica para evitar que o trabalho do LCD fique diretamente dentro desse ciclo.

## 13. Wi-Fi, IP, HTTP e JSON

SSID é o nome da rede. A senha fica no `config.h`. O IP identifica a placa dentro da rede local; pode mudar por DHCP. A porta identifica o serviço dentro daquele endereço.

O computador pode usar a rede de 5 GHz do mesmo roteador e a ESP32 usar 2,4 GHz se as duas redes permitirem comunicação local entre si. Não basta que duas redes tenham internet: elas precisam permitir o acesso aos endereços das placas. Redes de convidados ou com isolamento podem impedir isso.

HTTP organiza pedidos e respostas. GET consulta; POST envia uma ação. JSON é um formato de dados usado nas respostas. Ele não é a aparência da página: o JavaScript lê o JSON e atualiza os elementos visuais.

### Rotas da principal

| Rota | Método | Papel |
|---|---|---|
| `/` | GET | Entregar HTML/CSS/JS do painel |
| `/status` | GET | Estado atual, contadores, saúde e programação |
| `/eventos` | GET | Eventos retidos e sessão atual |
| `/cmd` | POST | Alterar modo, pausa, sirene ou programação |

Exemplo conceitual de resposta, com valores ilustrativos:

```json
{"armado":true,"modo":"monitoramento","total":12,"deteccoes":20}
```

Exemplos de campos enviados ao `/cmd`:

```text
c=modo&v=apresentacao
c=arm&v=0
c=sirene&v=0
c=noturno&v=1&inicio=22:00&fim=07:00&dia=monitoramento
```

`comando()` valida os valores. Responde 400 para entrada inválida, 503 se não consegue salvar uma preferência necessária e 409 no teste de som incompatível com Monitoramento. Uma resposta 200 indica o caminho de sucesso do pedido, acompanhado do estado atual.

### Descoberta da câmera

`procurarCamera()` tenta o IP configurado e depois endereços com o mesmo prefixo local, um de cada vez. `ehCamera()` consulta `/status` e procura a propriedade `framesize`. Isso é uma descoberta simples por formato de resposta, não autenticação criptográfica de uma câmera. A busca assume o modelo de sub-rede baseado na alteração do último octeto; não é uma descoberta universal para qualquer rede.

Depois que encontra a câmera, o estado `camEncontrada` interrompe essa busca. A implementação atual não monitora continuamente uma mudança posterior de IP da câmera para reiniciar essa descoberta. Esse é um limite real a mencionar se perguntarem sobre reconexão em qualquer cenário.

## 14. Por que há um servidor HTTP adaptado

`ServidorRastro` estende `WebServer`. Navegadores podem abrir conexões antes de mandar dados. Uma conexão vazia não deve monopolizar o atendimento enquanto outra já tem um pedido pronto.

A adaptação guarda até quatro conexões em espera, acompanha quando chegaram, procura primeiro as que já têm bytes disponíveis e descarta conexões ociosas após a janela prevista ou sob pressão de espaço. O parser e os handlers HTTP continuam sendo os da biblioteca oficial.

`setNoDelay(true)` aplica TCP_NODELAY no cliente aceito. Isso reduz o agrupamento deliberado de pequenas escritas pelo mecanismo conhecido como Nagle; não cria banda extra nem garante latência constante. A adaptação usa membros protegidos da biblioteca, por isso a versão do pacote ESP32 usada na validação, 3.3.12, é relevante para reproduzir o comportamento.

### Por que `pagina_http.h` existe

O painel cresceu para mais de 64 KB. Na configuração sem PSRAM da principal, a classe `String` dessa versão do core tem um limite de capacidade que impedia montar uma cópia inteira da página. A resposta podia chegar vazia.

`enviarPagina()` calcula o tamanho total, substitui o marcador `__CAM__` pelo IP e envia trechos diretamente do texto guardado na flash. Usa `setContentLength()` e `sendContent_P()`. Assim não precisa de uma `String` gigante para o HTML. Não se trata de compressão gzip nem de dividir o JavaScript em vários programas: é uma forma de enviar os bytes do mesmo documento.

## 15. A câmera: `CameraWebServer.ino`

O firmware da câmera deriva do exemplo CameraWebServer. Seu `setup()` configura a interface do sensor, inicializa a câmera, conecta o Wi-Fi e inicia os servidores. O `loop()` imprime o IP e o estado da rede a cada dez segundos; o streaming é atendido pelas tarefas do servidor, não por esse pequeno `loop()`.

### Configuração que efetivamente vale

O arquivo contém configurações preliminares herdadas do exemplo, como UXGA e alternativas sem PSRAM. Mais abaixo há um bloco que fixa os parâmetros usados pelo RASTRO. Portanto, ler só as primeiras atribuições leva a uma conclusão errada.

Na versão atual, a configuração efetiva exige PSRAM detectada, usa JPEG, QVGA de 320×240, memória de quadro em PSRAM e dois buffers com `CAMERA_GRAB_LATEST`, conforme a constante `RASTRO_TESTE_PSRAM_BUFFERS = 2`. Se não detecta PSRAM, o código retorna antes de iniciar o serviço de câmera.

A inicialização do sensor começa com XCLK de 20 MHz. Depois de `esp_camera_init()`, o programa aplica o clock validado de 8 MHz. XCLK é um sinal de clock fornecido à câmera; não é o FPS nem a frequência da CPU principal.

A qualidade JPEG começa em 18 no firmware, mas o painel aplica o perfil selecionado: **Mais fluido usa 40; Mais nítido usa 14**. Nessa API, um número de qualidade maior corresponde a maior compressão e menor qualidade visual. Portanto 40 não quer dizer “melhor imagem” que 14.

### Buffers e descarte de quadros antigos

Um buffer é uma área onde um quadro fica disponível. Dois buffers permitem organizar captura e consumo com mais flexibilidade, mas não duplicam automaticamente o FPS. `CAMERA_GRAB_LATEST` favorece quadros recentes, útil para reduzir atraso visual. Quadros antigos podem ser descartados; esse não é um sistema de gravação de todos os quadros.

### Um detalhe da versão atual

O código contém uma linha que desativa o detector de brownout. Isso não melhora a fonte de alimentação nem prova que uma alimentação fraca está adequada. Ao explicar, apresente como uma escolha presente no firmware e uma limitação a revisar em evolução de hardware; não diga que quedas de tensão foram resolvidas por software.

## 16. `app_httpd.cpp`: foto e vídeo pela rede

O servidor de controle e foto usa a porta 80. O de streaming usa a porta 81.

`capture_handler()` pede um quadro com `esp_camera_fb_get()`, responde com JPEG e devolve o buffer com `esp_camera_fb_return()`. Devolver o buffer é essencial: sem isso, a câmera pode ficar sem espaço para continuar capturando.

`stream_handler()` repete captura e envio. O formato é MJPEG: várias imagens JPEG dentro de uma resposta HTTP multipart. Cada parte tem cabeçalho e tamanho do JPEG. Não é H.264, não há arquivo de vídeo completo sendo gravado e não há áudio embutido.

`cmd_handler()` altera parâmetros do sensor, como qualidade e tamanho, por `/control`. `status_handler()` informa parâmetros do sensor e métricas. `xclk_handler()` permite ajustar o clock. `startCameraServer()` registra as rotas e inicia os dois serviços.

A instrumentação acrescentada mede quantidade de quadros, bytes do último quadro, tempo de captura e envio e estado de transmissão. Uma região crítica protege a cópia dessas métricas entre tarefas. TCP_NODELAY é aplicado às conexões dos servidores da câmera.

Os handlers de registradores, PLL, janela e BMP são recursos do exemplo de câmera para controle e diagnóstico; o painel RASTRO usa principalmente `/status`, `/capture`, `/control`, `/xclk` e `:81/stream`. Não é necessário dizer que cada controle avançado do exemplo é uma função central da instalação.

O cabeçalho `X-Framerate: 60` que aparece no exemplo não é uma medição do desempenho. O número mostrado no painel é calculado a partir dos quadros que ele conseguiu decodificar e exibir.

## 17. O painel: HTML, CSS e JavaScript

Em `pagina.h`, `R"rawliteral(...)rawliteral"` permite guardar um texto longo com aspas e quebras de linha sem escapar cada caractere. `PROGMEM` indica o armazenamento do texto na flash. O navegador recebe esse conteúdo pela rota `/`.

HTML descreve elementos como botões, tabelas e caixas. CSS define aparência e adaptação a telas menores. JavaScript busca dados, reage a ações e manipula o conteúdo desses elementos.

O auxiliar `$` encontra um elemento pelo ID. `textContent` escreve texto; `createElement()` cria um elemento; `append()` monta a árvore da interface. `addEventListener()` registra uma reação a um evento. `onclick` define o que acontece ao clicar.

### Programação assíncrona

`fetch()` inicia um pedido de rede. `async` e `await` permitem esperar o resultado sem escrever o fluxo como uma sequência de callbacks aninhados. Isso não congela toda a aba. `try/catch/finally` separa sucesso, tratamento de erro e limpeza que deve ocorrer nos dois casos.

`AbortController` permite cancelar uma operação. `setTimeout()` agenda uma execução futura; não significa que o navegador terá precisão de tempo real. As flags de operação e os números de geração impedem que respostas atrasadas sobrescrevam uma sessão nova de transmissão.

### Consulta do estado

`poll()` consulta `/status` e verifica se precisa buscar `/eventos`: mudança de sessão, revisão, sincronização de horário ou falta dessa informação. Depois atualiza os dados e agenda outra consulta, normalmente 1,5 s após terminar a anterior.

Esse valor não é “1,5 s garantido entre dados novos”: pedidos lentos levam mais tempo. A flag `busy` evita consultas de polling sobrepostas. Em falha, o painel indica dados desatualizados e tenta reconectar.

`setStatus()` atualiza indicadores, botões, modo, saúde e programação. `command()` envia as ações e espera confirmação. O modo noturno possui um formulário próprio com validação, preservação de edição ainda não salva e bloqueio de ações concorrentes durante o salvamento.

### Histórico, filtros e resumo

`allEvents()` combina eventos da placa, arquivos importados e metadados disponíveis com as fotos. `filtered()` aplica texto, tipo, presença de foto e datas. `historySlice()` seleciona 25 linhas para desenhar, preservando a posição de leitura quando entram registros mais recentes.

Paginação não apaga eventos. A exportação CSV usa todo o resultado filtrado, não só a página visível. O backup usa o conjunto disponível e não depende da página atual nem dos filtros.

`renderActivity()` calcula contagens do conjunto filtrado, fotos de ocorrências, média entre o primeiro e último sinal e distribuição por hora. Os botões de dias facilitam filtrar um dia. Eventos sem hora utilizável ficam fora dos gráficos por horário e dos filtros por data.

O resumo não é uma estatística completa de todos os dias porque a placa tem retenção limitada e os arquivos do navegador podem variar. Tampouco a média dos intervalos é a permanência média de pessoas no ambiente.

## 18. Como o navegador exibe o vídeo

`cameraConnect()` consulta a câmera, ajusta parâmetros quando apropriado e escolhe transmissão contínua ou recuperação por fotos. A versão atual respeita outro streaming ativo e evita mudar clock e qualidade do outro espectador nessa situação.

`CameraFrames` analisa a resposta multipart. Um pacote da rede não corresponde necessariamente a uma imagem completa: cabeçalhos e JPEGs podem chegar divididos. O parser acumula bytes, encontra o fim do cabeçalho, lê Content-Length e espera os bytes suficientes para reconstruir um JPEG. Há limites de tamanho para evitar crescimento indefinido.

`displayCameraFrame()` tenta decodificar o JPEG, verifica se pertence à geração atual da conexão, troca a imagem mostrada e libera a URL temporária anterior. `lastCameraFrame` guarda uma cópia recente para reaproveitar em captura de fotos.

O FPS exibido é aproximadamente:

```text
quadros exibidos × 1000 / tempo decorrido em milissegundos
```

O código pode descartar quadros intermediários se chegam vários de uma vez; portanto FPS de exibição, FPS de captura e quantidade de JPEGs enviados não são obrigatoriamente o mesmo número.

### Falhas e recuperação

Sem quadro novo por cerca de 3 s, aparece aviso de atraso. Um timeout mais longo cancela uma tentativa que não avança. As reconexões têm espera crescente e limitada. Após falhas, o painel pode usar fotos em sequência; tenta voltar ao vídeo depois de cerca de um minuto quando as condições permitem.

Pausar vídeo nesta aba não pausa automaticamente o PIR nem o histórico da placa. As fotos automáticas ainda podem exigir uma captura se configuradas, mesmo com o vídeo pausado.

### Por que uma cena preta pode mostrar FPS maior

Uma imagem simples costuma produzir JPEG menor. Uma cena com muitos detalhes pode exigir mais bytes; iluminação e exposição também alteram o tempo de captura. A transmissão, o processamento da câmera, a exposição, o navegador e a rede influenciam o resultado. Não é correto concluir apenas pelo contador que “a internet do celular” é a única causa.

Não há um FPS máximo universal garantido para este conjunto. O perfil fluido e a resolução foram ajustados para melhorar o uso, mas o desempenho precisa ser medido nas condições da apresentação. Modo noturno programado não acrescenta visão infravermelha nem elimina a necessidade de luz para uma imagem utilizável.

## 19. Fotos, álbuns e IndexedDB

`scheduleAlbum()` decide quando fotografar. Entre as condições estão: registro ligado, captura automática selecionada, página visível, ausência de outra captura em andamento, ocorrência elegível e espaço disponível.

Uma ocorrência pode ter até cinco fotos, com intervalo mínimo de aproximadamente quatro segundos entre tentativas. Há condições especiais para a primeira foto recente; as seguintes dependem de atividade do PIR e da ocorrência ainda aberta. O limite total é 300 fotos no navegador. Foto manual tem um limite de uma imagem por registro manual.

Essas regras significam **até cinco**, e não cinco garantidas. Um movimento curto, câmera indisponível, aba escondida ou limite de armazenamento pode resultar em menos fotos ou nenhuma.

`capture()` prefere reaproveitar um quadro de vídeo recente, da geração correta e com menos de 1,5 s. Se não há quadro adequado, interrompe sua conexão de vídeo para solicitar `/capture`, salva o JPEG e retoma o vídeo. Isso evita pedir uma captura extra quando a imagem desejada já está chegando pelo streaming.

`Blob` representa dados binários, como um JPEG. Uma Object URL permite exibir esse Blob em um elemento de imagem. `URL.revokeObjectURL()` libera o recurso temporário quando ele deixa de ser usado.

O banco é `rastro-fotos-v1`; a versão do banco é **3**, apesar do nome. Há os conjuntos `photos` e `archives`. O nome é uma identidade histórica do banco, não uma promessa de que ele continua no esquema 1.

`galleryRender()`, `galleryDetails()` e `scheduleGallery()` permitem navegar pelas fotos, ver miniaturas, horários e intervalos e reproduzir a sequência. A reprodução avança por fotos em intervalos de 1,4 s e para ao final; não reconstrói um vídeo contínuo do acontecimento.

Ao trocar de IP, o navegador passa a uma origem diferente. Por isso fotos associadas ao endereço anterior não aparecem automaticamente no novo endereço. O backup permite transportá-las. Dois navegadores ou dois computadores também não compartilham automaticamente o mesmo banco local.

## 20. Backup: o que contém e como a restauração evita perdas

`buildBackup()` produz JSON com `format: rastro-backup`, versão 1, data de criação, eventos e álbuns. As fotos JPEG são representadas por data URLs com base64. Base64 é codificação de bytes em texto; não é criptografia.

Limites previstos: até 128 MB de arquivo, 10 mil registros arquivados, 300 fotos, cinco fotos por ocorrência e uma por registro manual. Esses limites são guardas do programa, não garantia de espaço livre em qualquer navegador.

`parseBackup()` verifica estrutura, versão, tipos, IDs, datas, limites e JPEGs. `planBackup()` calcula fotos novas, repetidas e registros a acrescentar. A identidade considera dados do quadro; não basta confiar no nome do arquivo.

`commitBackup()` faz uma transação conjunta nas áreas de fotos e arquivos. Transação significa que as operações são confirmadas em conjunto ou abortadas. Também verifica se outra aba alterou fotos ou ocupou o limite enquanto a restauração era planejada.

`restoreBackup()` relê dados recentes, planeja, confirma e só depois atualiza o estado da interface. A restauração acrescenta ao arquivo do navegador; não reescreve os contadores físicos da principal. Dados atuais da placa têm prioridade na representação de um evento que também existe em arquivo.

CSV é outra coisa: exporta uma tabela para leitura em planilhas; não contém os JPEGs para reconstruir os álbuns. O ZIP de código enviado ao Lucas também não é o backup dos álbuns de uso.

## 21. Instalar e reproduzir a versão

1. Extraia o ZIP inteiro. Não abra somente o `.ino` ainda dentro do compactador.
2. Na pasta da principal e na da câmera, preencha `config.h` com a sua rede. Os arquivos enviados têm exemplos, não a senha da instalação do Arthur.
3. Use o pacote ESP32 da Espressif e a biblioteca LiquidCrystal I2C. A versão validada do pacote foi 3.3.12; isso é uma referência de reprodução, não uma afirmação de que é a versão mais recente disponível.
4. Na câmera: ESP32 Dev Module, Huge APP e PSRAM Enabled. O código seleciona AI-Thinker para a pinagem. Mantenha todos os arquivos da pasta.
5. Grave a câmera com o método de programação apropriado à montagem; abra a Serial em 115200 e anote o IP.
6. Coloque esse IP em `CAM_IP_PADRAO` da principal. A busca simples também tenta encontrá-la, mas configurar corretamente evita demora.
7. Na principal: ESP32 Dev Module, Huge APP e PSRAM Disabled. Mantenha o `partitions.csv` específico junto ao sketch.
8. Grave, aguarde a calibração e abra o IP do LCD ou da Serial no navegador com acesso à mesma rede local.
9. Ative a programação noturna pelo painel se for uma placa nova e confira o relógio mostrado.
10. Teste movimento, modos, fotos, CSV e backup antes da apresentação, nas mesmas condições de luz e rede que pretende usar.

Não grave o firmware da câmera na principal por engano: são dois sketches diferentes. O nome da porta COM indica uma porta no computador, não qual sketch deve ser enviado. A ponte TX/RX com a principal mantida em reset foi uma solução de gravação; depois retorna-se à montagem normal.

## 22. O que foi testado e o que você não deve prometer

Foram realizados testes da lógica de agrupamento e dos modos, preservação de histórico, paginação com muitos registros, álbuns e limites, migração e transações de IndexedDB, backup e concorrência entre abas, recuperação de vídeo, perfis de qualidade, envio de HTML grande e programação noturna.

Na programação noturna, o teste usou um horário temporário próximo, aguardou a mudança real do relógio, observou a principal entrar em Ausente, verificou uma escolha manual e restaurou 22h–7h. Depois um reinício confirmou horários salvos, relógio sincronizado, sensor calibrado e página completa.

Isso não transforma a montagem em equipamento de segurança certificado. Também não demonstra reconhecimento facial, identificação de pessoas, rastreamento individual, armazenamento de vídeo contínuo, fotos automáticas sem navegador, acesso remoto pela internet ou alertas no celular. Essas funções não estão implementadas nesta versão.

As consultas de rede já apresentaram oscilações em verificações anteriores. O usuário relatou o painel atualizando normalmente, e as últimas validações funcionaram; não é correto prometer que qualquer rede terá latência ou FPS constantes.

## 23. O que mostrar ao apresentar

Comece pelo problema e pela intenção da instalação, depois demonstre o fluxo de um movimento. Mostre que o LCD reage, o painel apresenta a ocorrência e as fotos ficam agrupadas. Explique por que duas detecções próximas podem pertencer ao mesmo acontecimento.

Em seguida, mostre os três modos e deixe claro que silêncio não significa que parou de registrar. Demonstre o botão de pausa. Abra o histórico, busque um registro, explique os filtros e mostre que o resumo usa o histórico disponível.

Abra uma sequência de fotos e explique a diferença entre essa sequência e vídeo. Mostre o backup como forma de levar fotos para outro navegador. Termine mostrando a programação noturna e onde os dados ficam.

Para mudar modos durante uma demonstração fora do horário previsto, lembre que uma escolha manual vale até a próxima transição programada. Evite deixar um teste temporário de horários ativo depois da demonstração.

## 24. Perguntas prováveis e respostas que você deve saber construir

**Por que duas placas?** A principal cuida da lógica do objeto e a câmera fornece JPEGs por um servidor próprio. O navegador combina os dois fluxos. A divisão evita fazer a principal retransmitir todo o vídeo.

**É uma contagem de pessoas?** Não. Há contagem de detecções e agrupamentos temporais de movimento. O sensor não identifica indivíduos.

**Se fechar o site, para tudo?** Não. Registro, modos, sirene, LCD, histórico e programação continuam na principal alimentada. As fotos automáticas dependem do navegador aberto e visível.

**Onde ficam as fotos?** No IndexedDB do navegador para aquele endereço e perfil. Os eventos e preferências ficam na flash da principal, em NVS.

**Se faltar energia, perde tudo?** O que foi confirmado na NVS deve ser recuperado; alterações ainda pendentes podem se perder. O relógio precisa sincronizar após uma inicialização sem hora conhecida. As fotos dependem do armazenamento do navegador, não da alimentação da principal.

**Por que só 100 eventos?** É uma retenção limitada e previsível no armazenamento da placa. Eventos novos substituem antigos. O contador acumulado não equivale à quantidade de linhas retidas.

**Por que o FPS varia?** Captura/exposição, quantidade de bytes por JPEG, processamento, rede e exibição variam. Uma cena preta e simples não é um teste representativo de uma sala iluminada com detalhes.

**Qual é o papel do JavaScript?** Consultar as APIs, atualizar a interface, decodificar/exibir quadros e gerenciar fotos, álbuns e backup no navegador.

**O que o CRC faz?** Verifica integridade de dados persistidos. Não identifica pessoas e não criptografa as imagens.

**O noturno melhora a visão da câmera?** Não. Ele programa modos e horários. Para imagem no escuro seria necessário resolver iluminação/sensor apropriado; não foi implementada visão noturna por infravermelho.

**Por que existe HTML dentro de um arquivo C++?** Para que a própria principal possa entregar o painel sem depender de um servidor de hospedagem externo.

**O sistema está protegido por senha de acesso ao painel?** O código atual não implementa autenticação de usuários para os comandos HTTP. O uso previsto é a rede local controlada da demonstração; não diga que há contas, login ou criptografia de ponta a ponta implementados.

## 25. Como estudar sem tentar decorar todas as linhas

Primeiro explique em voz alta o caminho sensor → principal → rede → navegador/câmera. Depois faça uma ocorrência de papel: desenhe um sinal alto/baixo e conte detecções e agrupamentos. Em seguida leia `setup()` e `loop()`, relacionando cada chamada ao desenho.

Leia os cabeçalhos nesta ordem: `ocorrencia.h`, `modos.h`, `historico.h`, `persistencia.h`, `noturno.h`, `pagina_http.h`. Depois leia o `setup()` da câmera e os handlers usados em `app_httpd.cpp`. Só então percorra o JavaScript por grupos: polling, renderização, vídeo, fotos, backup e programação.

Use o arquivo `MAPA-DO-CODIGO.md` para localizar funções e linhas. O `CODIGO-PARA-LEITURA.html` reúne os arquivos com números de linha e busca de texto do navegador. O código compilável continua nas duas pastas de sketch; os arquivos de estudo são auxiliares.

Você entendeu um módulo quando consegue responder: quais dados ele recebe, qual estado mantém, o que altera, onde grava e o que faz se falhar. Essa é uma preparação mais útil que memorizar símbolos isolados.

## 26. Exercícios para treinar a explicação

1. Desenhe sinais em 0 s, 3 s e 20 s e explique em quais situações eles pertencem a uma ou mais ocorrências. Inclua o tempo em que o PIR mantém HIGH.
2. Explique por que uma pessoa parada diante da câmera não é necessariamente uma nova detecção do PIR.
3. Diga o que continua funcionando ao fechar a aba e o que deixa de ser capturado.
4. Compare NVS, PSRAM e IndexedDB sem usar a palavra “memória” como se os três fossem a mesma coisa.
5. Calcule 22:00 e 07:00 em minutos e explique por que a condição usa OU ao atravessar a meia-noite.
6. Explique o que acontece ao escolher Monitoramento manualmente às 23h e reiniciar antes das 7h.
7. Explique por que `delay(8000)` seria inadequado no ciclo principal para controlar o alarme.
8. Mostre em qual arquivo o IP padrão da câmera é configurado e de onde sai o IP da principal.
9. Diferencie CSV, backup JSON e ZIP de código.
10. Explique por que “X-Framerate: 60” não comprova uma transmissão de 60 FPS.

Se conseguir responder a esses exercícios relacionando cada resposta a uma parte do código, você estará preparado para explicar a programação com entendimento, em vez de apenas repetir a descrição do projeto.

## 27. Símbolos e decisões que costumam gerar dúvidas na leitura

### Lendo a expressão que autoriza uma detecção

```cpp
bool leitura = digitalRead(PIN_PIR) == HIGH;
bool ativo = leitura && armado && agora >= AQUECIMENTO;
```

A primeira linha transforma a leitura do pino em verdadeiro ou falso. A segunda exige três coisas ao mesmo tempo: o sinal deve estar alto, o registro deve estar ligado e a calibração deve ter terminado. Portanto um HIGH sozinho ainda não garante um registro.

```cpp
if (ativo && !anterior) {
    ++totalMovimentos;
}
anterior = ativo;
```

Se o sinal está ativo agora e não estava antes, houve uma subida. Ao guardar o estado atual em `anterior`, a próxima volta consegue fazer a comparação. Esse é um exemplo simples de estado entre execuções do ciclo; sem ele, um sinal alto poderia ser contado repetidamente.

Uma variável global, declarada fora das funções, continua disponível entre as chamadas e pode ser usada por várias funções. Uma variável local comum existe no contexto de uma chamada. Uma variável local `static` conserva seu valor entre chamadas; por isso ela aparece nos controles de intervalo de publicação do LCD e da programação.

### Vetores, índices e referências

`eventos[proximo]` seleciona uma posição do vetor. `Evento &e = eventos[proximo]` cria uma referência para trabalhar nessa posição sem copiar o evento. O vetor tem tamanho fixo; usar um índice válido é responsabilidade da lógica. O `% LIMITE` faz o índice voltar ao início na fila circular.

### Bits não são as mesmas operações dos booleanos

No noturno aparece uma ideia como:

```cpp
b[8] = (habilitado ? 1 : 0) | (uint8_t(dia) << 1);
```

`<< 1` desloca bits uma posição à esquerda. O operador `|`, com uma barra, combina bits. O operador `||`, com duas barras, é o OU lógico entre condições. São operações diferentes.

O byte reúne uma flag de ativação e os bits do modo diurno. Ao recuperar, máscaras e deslocamentos separam esses valores. `0x` indica escrita hexadecimal: por exemplo, `0x80` representa o bit mais alto de um byte. As tabelas de partição usam números hexadecimais para endereços e tamanhos, não nomes mágicos de arquivos.

### Os três relógios que aparecem no painel

`Date.now()` no navegador representa um instante de calendário em milissegundos e é usado, por exemplo, para datar fotos. `performance.now()` mede intervalos relativos de execução da página e é usado para FPS, idade de quadros e tentativas. `millis()` pertence à placa e mede o tempo desde sua inicialização. `time(nullptr)` na placa fornece o horário Unix quando sincronizado. Não se deve trocar esses valores indiscriminadamente: eles não têm necessariamente a mesma origem ou a mesma unidade.

### Map, Set, Promise e callbacks

No JavaScript, `Map` associa uma chave a um valor, como identidade da ocorrência → álbum. `Set` guarda itens únicos, útil para conferir duplicados. Um Array é uma lista ordenada. `filter()` seleciona itens, `map()` transforma itens e `reduce()` acumula um resultado, como a contagem total de fotos.

Uma `Promise` representa uma operação que pode terminar depois, com sucesso ou erro. Os eventos `onsuccess`, `oncomplete` e `onabort` são callbacks do armazenamento. Por isso o programa espera a transação confirmar antes de tratar a restauração como concluída; pedir uma gravação não é o mesmo que confirmar que ela terminou.

### Quando o pré-processador escolhe um caminho

`#if`, `#ifdef` e `#define` são avaliados na compilação. As alternativas de câmera para outros modelos não ficam todas rodando ao mesmo tempo. Um `if` comum, por outro lado, decide durante a execução. A seleção AI-Thinker é de compilação; o teste de Wi-Fi conectado é de execução. Essa diferença ajuda a ler o exemplo de câmera sem imaginar que cada bloco se aplica à placa atual.
