# app.py
from flask import Flask, render_template, request, jsonify, redirect, url_for, session, Response
from flask_socketio import SocketIO, emit
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime, timedelta
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
    origen = db.Column(db.String(20), nullable=False, default='web')

# Crear tablas
with app.app_context():
    db.create_all()

# Colores disponibles
COLORES_BOTELLAS = [
    'Rojo',
    'Verde',
    'Azul',
    'Amarillo',
    'Blanco',
    'Negro'
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
        print("Recibiendo datos:", data)
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
        
        print("Enviando respuesta")
        # Emitir respuesta al cliente que hizo la solicitud
        emit('respuesta_registro', {
            'estado': nuevo_registro.estado,
            'registros': registros_data,
            'total_registros': len(registros_sesion)
        })

        # Emitir actualización a todos
        socketio.emit('database_updated', {
            'type': 'new_registro',
            'data': {
                'nfc_id': nuevo_registro.nfc_id,
                'usuario': nuevo_registro.usuario,
                'color': nuevo_registro.color,
                'estado': nuevo_registro.estado,
                'origen': nuevo_registro.origen,
                'timestamp': nuevo_registro.timestamp.strftime('%Y-%m-%d %H:%M:%S')
            }
        })  # True para broadcast
        
    except Exception as e:
        print("Error:", str(e))
        emit('error_registro', {'error': str(e)})

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
        origen = data.get('origen', '--')

        # Verificar si el NFC ya existe
        existe = Registro.query.filter_by(nfc_id=nfc_id).first()
        
        nuevo_registro = Registro(
            usuario=usuario,
            nfc_id=nfc_id,
            color=color,
            estado='nfc_duplicado' if existe else 'nfc_OK',
            origen=origen
        )
        
        db.session.add(nuevo_registro)
        db.session.commit()

        # Emitir actualización a todos
        socketio.emit('database_updated', {
            'type': 'new_registro',
            'data': {
                'nfc_id': nuevo_registro.nfc_id,
                'usuario': nuevo_registro.usuario,
                'color': nuevo_registro.color,
                'estado': nuevo_registro.estado,
                'origen': nuevo_registro.origen,
                'timestamp': nuevo_registro.timestamp.strftime('%Y-%m-%d %H:%M:%S')
            }
        })

        return jsonify({
            'estado': nuevo_registro.estado,
            'mensaje': 'NFC duplicado' if existe else 'Registro exitoso'
        })

    except Exception as e:
        return jsonify({
            'estado': 'error',
            'mensaje': str(e)
        }), 500

@app.route('/verListadoUsuario')
def ver_listado():
    usuarios = db.session.query(Registro.usuario).distinct().all()
    resumen = []
    
    for user in usuarios:
        registros = Registro.query.filter_by(usuario=user[0]).all()
        total = len(registros)
        duplicados = len([r for r in registros if r.estado == 'nfc_duplicado'])
        
        # Conteo dinámico por cada color
        conteo_colores = {}
        for color in COLORES_BOTELLAS:
            conteo_colores[color.lower()] = len([r for r in registros if r.color == color])
        
        resumen.append({
            'usuario': user[0],
            'total': total,
            'duplicados': duplicados,
            **conteo_colores  # Agrega todos los conteos de colores al diccionario
        })
    
    return render_template('listado.html', resumen=resumen, colores=COLORES_BOTELLAS)

@app.route('/verTodo')
def ver_todo():
    registros = Registro.query.order_by(Registro.timestamp.desc()).all()
    return render_template('ver_todo.html', registros=registros)

@app.route('/api/user_stats/<usuario>')
def get_user_stats(usuario):
    registros = Registro.query.filter_by(usuario=usuario).all()
    total = len(registros)
    duplicados = len([r for r in registros if r.estado == 'nfc_duplicado'])
    
    # Conteo dinámico por cada color
    conteo_colores = {}
    for color in COLORES_BOTELLAS:
        conteo_colores[color.lower()] = len([r for r in registros if r.color == color])
    
    return jsonify({
        'usuario': usuario,
        'total': total,
        'duplicados': duplicados,
        **conteo_colores  # Agrega todos los conteos de colores al JSON
    })

@app.route('/descargar_csv')
def descargar_csv():
    si = StringIO()
    cw = csv.writer(si)
    
    # Encabezados dinámicos basados en los colores configurados
    headers = ['Timestamp', 'Usuario', 'NFC ID'] + COLORES_BOTELLAS + ['Estado', 'Origen']
    cw.writerow(headers)
    
    # Obtener todos los registros ordenados por fecha
    registros = Registro.query.order_by(Registro.timestamp.desc()).all()
    
    # Escribir los registros
    for registro in registros:
        row = [
            registro.timestamp.strftime('%Y-%m-%d %H:%M:%S'),
            registro.usuario,
            registro.nfc_id
        ]
        
        # Agregar 1 o 0 para cada color según corresponda
        for color in COLORES_BOTELLAS:
            row.append(1 if registro.color == color else 0)
            
        row.extend([
            registro.estado,
            registro.origen
        ])
        
        cw.writerow(row)
    
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

@app.route('/api/stats')
def get_stats():
    # Estadísticas globales
    total_registros = Registro.query.count()
    total_duplicados = Registro.query.filter_by(estado='nfc_duplicado').count()
    
    # Estadísticas por color
    stats_colores = {}
    for color in COLORES_BOTELLAS:
        stats_colores[color] = Registro.query.filter_by(color=color).count()
    
    # Si hay usuario en sesión, obtener sus estadísticas
    user_stats = None
    if 'usuario' in session:
        registros_usuario = Registro.query.filter_by(usuario=session['usuario']).all()
        user_stats = {
            'total': len(registros_usuario),
            'duplicados': len([r for r in registros_usuario if r.estado == 'nfc_duplicado']),
            'colores': {}
        }
        for color in COLORES_BOTELLAS:
            user_stats['colores'][color] = len([r for r in registros_usuario if r.color == color])
    
    return jsonify({
        'total_registros': total_registros,
        'total_duplicados': total_duplicados,
        'stats_colores': stats_colores,
        'user_stats': user_stats
    })
    # Total de registros
    total_registros = Registro.query.count()
    
    # Registros de hoy
    hoy = datetime.now().date()
    registros_hoy = Registro.query.filter(
        db.func.date(Registro.timestamp) == hoy
    ).count()
    
    # Usuarios activos (con registros en las últimas 24 horas)
    hace_24h = datetime.now() - timedelta(days=1)
    usuarios_activos = db.session.query(Registro.usuario).distinct().filter(
        Registro.timestamp >= hace_24h
    ).count()
    
    return jsonify({
        'total_registros': total_registros,
        'registros_hoy': registros_hoy,
        'usuarios_activos': usuarios_activos
    })

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)