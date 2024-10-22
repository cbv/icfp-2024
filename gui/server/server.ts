import { writeFileSync } from 'fs';
import * as fs from 'fs';
import express from 'express';
import * as path from 'path';
import { spawn } from 'node:child_process';
import { glob } from 'glob';
import { EvalThreedRpc, PuzzleSolution } from '../src/types';

const token = fs.readFileSync(path.join(__dirname, '../../API_KEY'), 'utf8').replace(/\n/g, '');
const app = express();
app.use(express.json());
app.post('/export', (req, res) => {
  let data = '';
  req.setEncoding('utf8');
  req.on('data', chunk => {
    data += chunk;
  });
  req.on('end', () => {
    const body = JSON.parse(data);
    writeFileSync(path.join(__dirname, '../data.json'), JSON.stringify(body, null, 2) + '\n', 'utf8');
    console.log('Wrote data to data.json');
    res.end('ok');
  });
});

app.use('/', express.static(path.join(__dirname, '../public')));

app.get('/solutions/threed', async (req, res) => {
  const files = await glob(path.join(__dirname, '../../solutions/threed/threed*.txt'))
  const goodFiles: PuzzleSolution[] = files
    .filter(file => file.match(/\/threed.*\.txt$/))
    .sort()
    .map(file => ({ name: path.basename(file), body: fs.readFileSync(file, 'utf8') }));
  res.json(goodFiles);
});

app.use('/solutions', express.static(path.join(__dirname, '../../solutions')));

app.post('/api/space', async (req, res) => {
  const body = req.body.rawString;
  const preq = new Request("https://boundvariable.space/communicate", {
    method: 'POST',
    headers: { 'Authorization': `Bearer ${token}` },
    body
  });
  const presp = await fetch(preq);
  const ptext = await presp.text();
  res.end(ptext);
});

app.post('/api/eval', async (req, res) => {
  const body = req.body.rawString;

  const child = spawn(path.join(__dirname, '../../cc/eval.exe'), []);
  child.stdin.write(body);
  child.stdin.end();

  let output = '';
  child.stdout.on('data', (data) => {
    output += data;
  });

  child.stderr.on('data', (data) => {
    output += data;
  });

  child.on('close', (code) => {
    res.end(output);
  });

});

app.post('/api/eval-threed', async (req, res) => {
  const body = req.body as EvalThreedRpc;

  // XXX raise this limit maybe
  const LIMIT = '200';

  const child = spawn(path.join(__dirname, '../../seaplusplus/3v'), ['--limit', LIMIT, '--json', body.a + '', body.b + '']);
  child.stdin.write(body.program);
  child.stdin.end();

  let output = '';
  child.stdout.on('data', (data) => {
    output += data;
  });

  child.stderr.on('data', (data) => {
    output += data;
  });

  child.on('close', (code) => {
    res.end(output); // this is sending json text of type EvalThreedResponse
  });

});

const PORT = process.env.PORT == undefined ? 8000 : parseInt(process.env.PORT);

app.listen(PORT, '127.0.0.1', function() {
  console.log(`listening on port ${PORT}`);
});
