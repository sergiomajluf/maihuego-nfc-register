# app.py
from flask import Flask, render_template, request, jsonify, redirect, url_for, session, Response
from flask_socketio import SocketIO, emit
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import csv
from io import StringIO

app = Flask(__name__)
app.config['SECRET_KEY'] = 'clave_secreta_cambiar_en_produccion'
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///nfc_registros.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)
socketio = SocketIO(app, cors_allowed_origins="*")

# Modelo de datos
class Registro(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)
    usuario = db.Column(db.String(100), nullable=False)
    nfc_id = db.Column(db.String(100), nullable=False)
    color = db.Column(db.String(50), nullable=False)
    estado = db.Column(db.String(20), nullable=False)
    origen = db.Column(db.String(20), nullable=False, default='web')  # 'web' o 'esp32'

# Crear tablas
with app.app_context():
    db.create_all()

# Colores disponibles
COLORES_BOTELLAS = [
    'Rojo',
    'Verde',
    'Azul',
    'Amarillo'
]

@app.route('/')
def index():
    return render_template('index.html', colores=COLORES_BOTELLAS)

@app.route('/login', methods=['POST'])
def login():
    color = request.form['color']
    if not color:
        return redirect(url_for('index'))
        
    if 'usuario' in session:
        # Si ya hay sesión, solo actualizamos el color
        session['color'] = color
        return redirect(url_for('registrar_nfc'))
    else:
        # Nuevo login
        usuario = request.form['usuario']
        if not usuario:
            return redirect(url_for('index'))
        session['usuario'] = usuario
        session['color'] = color
        return redirect(url_for('registrar_nfc'))

@app.route('/registrarNFC')
def registrar_nfc():
    if 'usuario' not in session:
        return redirect(url_for('index'))
    usuario = session.get('usuario')
    color = session.get('color')
    # Obtener registros actuales del usuario
    registros = Registro.query.filter_by(usuario=usuario).order_by(Registro.timestamp.desc()).all()
    return render_template('registrar_nfc.html', 
                         usuario=usuario,
                         color=color,
                         registros=registros,
                         total_registros=len(registros))

@app.route('/logout')
def logout():
    session.clear()
    return redirect(url_for('index'))

@socketio.on('connect')
def handle_connect():
    print('Cliente conectado')

@socketio.on('disconnect')
def handle_disconnect():
    print('Cliente desconectado')

@socketio.on('registrar_nfc')
def handle_nfc_registro(data):
    try:
        print("Recibiendo datos:", data)  # Debug
        nfc_id = data.get('nfc_id')
        usuario = data.get('usuario')
        color = data.get('color')

        if not all([nfc_id, usuario, color]):
            raise ValueError("Faltan datos requeridos")

        # Verificar si el NFC ya existe
        existe = Registro.query.filter_by(nfc_id=nfc_id).first()
        
        nuevo_registro = Registro(
            usuario=usuario,
            nfc_id=nfc_id,
            color=color,
            estado='nfc_duplicado' if existe else 'nfc_OK',
            origen='web'
        )
        
        db.session.add(nuevo_registro)
        db.session.commit()
        
        # Obtener registros actualizados
        registros_sesion = Registro.query.filter_by(usuario=usuario).order_by(Registro.timestamp.desc()).all()
        
        # Preparar datos para enviar
        registros_data = [{
            'nfc_id': r.nfc_id,
            'estado': r.estado,
            'color': r.color,
            'origen': r.origen,
            'timestamp': r.timestamp.strftime('%Y-%m-%d %H:%M:%S')
        } for r in registros_sesion]
        
        print("Enviando respuesta")  # Debug
        emit('respuesta_registro', {
            'estado': nuevo_registro.estado,
            'registros': registros_data,
            'total_registros': len(registros_sesion)
        })
        
    except Exception as e:
        print("Error:", str(e))  # Debug
        emit('error_registro', {'error': str(e)})

@app.route('/verListadoUsuario')
def ver_listado():
    usuarios = db.session.query(Registro.usuario).distinct().all()
    resumen = []
    
    for user in usuarios:
        registros = Registro.query.filter_by(usuario=user[0]).all()
        total = len(registros)
        ok = len([r for r in registros if r.estado == 'nfc_OK'])
        duplicados = total - ok
        web = len([r for r in registros if r.origen == 'web'])
        esp32 = len([r for r in registros if r.origen == 'esp32'])
        
        resumen.append({
            'usuario': user[0],
            'total': total,
            'ok': ok,
            'duplicados': duplicados,
            'web': web,
            'esp32': esp32
        })
    
    return render_template('listado.html', resumen=resumen)

@app.route('/verTodo')
def ver_todo():
    registros = Registro.query.order_by(Registro.timestamp.desc()).all()
    return render_template('ver_todo.html', registros=registros)

@app.route('/descargar_csv')
def descargar_csv():
    si = StringIO()
    cw = csv.writer(si)
    
    # Escribir el encabezado
    cw.writerow(['Timestamp', 'Usuario', 'NFC ID', 'Color', 'Estado', 'Origen'])
    
    # Obtener todos los registros ordenados por fecha
    registros = Registro.query.order_by(Registro.timestamp.desc()).all()
    
    # Escribir los registros
    for registro in registros:
        cw.writerow([
            registro.timestamp.strftime('%Y-%m-%d %H:%M:%S'),
            registro.usuario,
            registro.nfc_id,
            registro.color,
            registro.estado,
            registro.origen
        ])
    
    # Obtener el contenido y cerrar el StringIO
    output = si.getvalue()
    si.close()
    
    # Crear el nombre del archivo con timestamp
    filename = f'registros_nfc_{datetime.now().strftime("%Y%m%d_%H%M%S")}.csv'
    
    # Devolver la respuesta
    return Response(
        output.encode('utf-8-sig'),
        mimetype='text/csv',
        headers={
            'Content-Disposition': f'attachment; filename={filename}',
            'Content-Type': 'text/csv; charset=utf-8'
        }
    )

@app.route('/api/registrar_nfc', methods=['POST'])
def api_registrar_nfc():
    try:
        data = request.get_json()
        
        if not data or 'nfc_id' not in data or 'usuario' not in data or 'color' not in data:
            return jsonify({
                'estado': 'error',
                'mensaje': 'Faltan datos requeridos'
            }), 400

        nfc_id = data['nfc_id']
        usuario = data['usuario']
        color = data['color']

        # Verificar si el NFC ya existe
        existe = Registro.query.filter_by(nfc_id=nfc_id).first()
        
        nuevo_registro = Registro(
            usuario=usuario,
            nfc_id=nfc_id,
            color=color,
            estado='nfc_duplicado' if existe else 'nfc_OK',
            origen='esp32'
        )
        
        db.session.add(nuevo_registro)
        db.session.commit()

        return jsonify({
            'estado': nuevo_registro.estado,
            'mensaje': 'NFC duplicado' if existe else 'Registro exitoso'
        })

    except Exception as e:
        return jsonify({
            'estado': 'error',
            'mensaje': str(e)
        }), 500

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)